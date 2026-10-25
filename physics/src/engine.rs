//! Thread de física a passo fixo. Publica snapshots (prev, curr) atrás de um
//! Mutex; quem lê (o C++) interpola entre eles.

use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

use crate::state::PhysicsState;
use crate::world::World;

struct Inner {
    world: World,
    prev: PhysicsState,
    curr: PhysicsState,
    curr_at: Instant, // quando `curr` foi publicado
    steps: u64,
    frozen: bool, // congelada no ponto mais alto
}

impl Inner {
    fn new(g: f64, h0: f64, theta1: f64, theta2: f64) -> Self {
        let world = World::new(g, h0, theta1, theta2);
        let st = world.state();
        Inner {
            world,
            prev: st,
            curr: st,
            curr_at: Instant::now(),
            steps: 0,
            frozen: false,
        }
    }
}

struct Shared {
    inner: Mutex<Inner>,
    quit: AtomicBool,
    dt: f64,
}

impl Shared {
    fn lock(&self) -> MutexGuard<'_, Inner> {
        self.inner.lock().unwrap_or_else(|e| e.into_inner())
    }
}

/// Dono da thread de física. Ao ser destruído, para a thread e espera ela terminar.
pub struct Engine {
    shared: Arc<Shared>,
    handle: Option<JoinHandle<()>>,
}

impl Engine {
    pub fn spawn(g: f64, h0: f64, theta1: f64, theta2: f64, dt: f64) -> Option<Engine> {
        let shared = Arc::new(Shared {
            inner: Mutex::new(Inner::new(g, h0, theta1, theta2)),
            quit: AtomicBool::new(false),
            dt: dt.max(1e-4),
        });
        let for_thread = Arc::clone(&shared);
        let handle = thread::Builder::new()
            .name("physics".into())
            .spawn(move || physics_thread(for_thread))
            .ok()?;
        Some(Engine {
            shared,
            handle: Some(handle),
        })
    }

    /// Recomeça a simulação (também descongela).
    pub fn reset(&self, g: f64, h0: f64, theta1: f64, theta2: f64) {
        *self.shared.lock() = Inner::new(g, h0, theta1, theta2);
    }

    /// (anterior, atual, alpha em 0..1)
    pub fn snapshots(&self) -> (PhysicsState, PhysicsState, f64) {
        let g = self.shared.lock();
        let alpha = if g.frozen {
            1.0
        } else {
            (g.curr_at.elapsed().as_secs_f64() / self.shared.dt).clamp(0.0, 1.0)
        };
        (g.prev, g.curr, alpha)
    }
}

impl Drop for Engine {
    fn drop(&mut self) {
        self.shared.quit.store(true, Ordering::Release);
        if let Some(h) = self.handle.take() {
            let _ = h.join();
        }
    }
}

fn physics_thread(shared: Arc<Shared>) {
    let dt = shared.dt;
    let period = Duration::from_secs_f64(dt);
    let mut next = Instant::now() + period;

    while !shared.quit.load(Ordering::Acquire) {
        {
            let mut g = shared.lock();
            if !g.frozen {
                let apex = g.world.step(dt, true);
                g.steps += 1;
                let mut st = g.world.state();
                st.steps = g.steps;
                if apex {
                    st.apex = 1;
                    g.frozen = true;
                }
                let old = g.curr;
                g.prev = old;
                g.curr = st;
                g.curr_at = Instant::now();
            }
        }

        // agenda por instantes absolutos: o erro do sleep não se acumula
        let now = Instant::now();
        if next > now {
            thread::sleep(next - now);
        }
        next += period;
        let now2 = Instant::now();
        if now2 > next + period * 5 {
            next = now2 + period; // muito atrasada: reancora, sem rajada de passos
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn thread_avanca_e_conserva_energia() {
        let e = Engine::spawn(
            9.81,
            5.0,
            45f64.to_radians(),
            30f64.to_radians(),
            1.0 / 120.0,
        )
        .expect("falha ao criar a thread");
        thread::sleep(Duration::from_millis(500));
        let (_prev, curr, alpha) = e.snapshots();
        assert!(curr.steps >= 20, "passos: {}", curr.steps);
        assert!((0.0..=1.0).contains(&alpha));
        assert!((curr.energy - 9.81 * 5.0).abs() < 1e-8);
        // ao sair do escopo, Drop para a thread e faz o join
    }
}
