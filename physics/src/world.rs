//! Física pura: uma partícula em um caminho de dois planos (esq -> vale -> dir).
//! s < 0: plano esquerdo | s = 0: vale | s > 0: plano direito.

use std::f64::consts::FRAC_PI_2;

use crate::state::PhysicsState;

const EPS: f64 = 1e-12;

pub(crate) struct World {
    g: f64,
    sin1: f64,
    cos1: f64,
    sin2: f64,
    cos2: f64,
    s: f64,
    v: f64,
    t: f64,
}

/// Menor raiz t > EPS de  s + v*t + a*t²/2 = 0
fn first_root(s: f64, v: f64, a: f64) -> Option<f64> {
    if a.abs() < 1e-12 {
        if v.abs() < 1e-12 {
            return None;
        }
        let t = -s / v;
        return (t > EPS).then_some(t);
    }
    let disc = v * v - 2.0 * a * s;
    if disc < 0.0 {
        return None;
    }
    let sq = disc.sqrt();
    let r1 = (-v - sq) / a;
    let r2 = (-v + sq) / a;
    [r1, r2]
        .into_iter()
        .filter(|&t| t > EPS)
        .fold(None, |acc: Option<f64>, t| {
            Some(acc.map_or(t, |m| m.min(t)))
        })
}

impl World {
    pub(crate) fn new(g: f64, h0: f64, theta1: f64, theta2: f64) -> Self {
        let th1 = theta1.clamp(1e-3, FRAC_PI_2 - 1e-3);
        let th2 = theta2.clamp(0.0, FRAC_PI_2 - 1e-3);
        let (sin1, cos1) = th1.sin_cos();
        let (sin2, cos2) = th2.sin_cos();
        let l1 = h0 / sin1; // comprimento do plano esquerdo até a altura h0
        World {
            g,
            sin1,
            cos1,
            sin2,
            cos2,
            s: -l1,
            v: 0.0,
            t: 0.0,
        }
    }

    fn on_left(&self) -> bool {
        self.s < 0.0 || (self.s == 0.0 && self.v < 0.0)
    }

    fn accel(&self) -> f64 {
        if self.on_left() {
            self.g * self.sin1
        } else {
            -self.g * self.sin2
        }
    }

    /// Avança `dt`. Com `stop_at_apex`, se a bola parar de subir no plano 2
    /// dentro do passo, ela é posicionada exatamente no ponto mais alto
    /// (v = 0) e a função devolve true.
    pub(crate) fn step(&mut self, dt: f64, stop_at_apex: bool) -> bool {
        let mut remaining = dt;
        for _ in 0..8 {
            let a = self.accel();

            if stop_at_apex && !self.on_left() && self.v > 0.0 && a < 0.0 {
                let t_apex = -self.v / a;
                if t_apex <= remaining {
                    self.s += self.v * t_apex + 0.5 * a * t_apex * t_apex;
                    self.v = 0.0;
                    self.t += t_apex;
                    return true;
                }
            }

            match first_root(self.s, self.v, a) {
                // atravessa o vale dentro deste passo: divide o passo
                Some(t) if t < remaining => {
                    self.v += a * t;
                    self.s = 0.0;
                    self.t += t;
                    remaining -= t;
                }
                _ => {
                    self.s += self.v * remaining + 0.5 * a * remaining * remaining;
                    self.v += a * remaining;
                    self.t += remaining;
                    return false;
                }
            }
        }
        false
    }

    pub(crate) fn state(&self) -> PhysicsState {
        let (tx, ty) = if self.on_left() {
            (self.cos1, -self.sin1)
        } else {
            (self.cos2, self.sin2)
        };
        let (x, y) = if self.s < 0.0 {
            (self.s * self.cos1, -self.s * self.sin1)
        } else {
            (self.s * self.cos2, self.s * self.sin2)
        };
        PhysicsState {
            s: self.s,
            v: self.v,
            x,
            y,
            tx,
            ty,
            speed: self.v.abs(),
            height: y,
            energy: 0.5 * self.v * self.v + self.g * y,
            time: self.t,
            ..Default::default()
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn run(theta2_deg: f64, secs: f64) -> (f64, f64) {
        let mut w = World::new(9.81, 5.0, 45f64.to_radians(), theta2_deg.to_radians());
        let e0 = w.state().energy;
        let (mut max_h, mut max_drift) = (0.0f64, 0.0f64);
        for _ in 0..(secs * 120.0) as usize {
            w.step(1.0 / 120.0, false);
            let st = w.state();
            max_h = max_h.max(st.height);
            max_drift = max_drift.max((st.energy - e0).abs());
        }
        (max_h, max_drift)
    }

    #[test]
    fn energia_conservada() {
        for ang in [60.0, 45.0, 30.0, 15.0, 5.0] {
            let (_, drift) = run(ang, 60.0);
            assert!(drift < 1e-8, "deriva de energia {drift} em {ang} graus");
        }
    }

    #[test]
    fn bola_volta_a_altura_inicial() {
        let (max_h, _) = run(30.0, 60.0);
        assert!((max_h - 5.0).abs() < 1e-6);
    }

    #[test]
    fn plano_horizontal_nao_desacelera() {
        let mut w = World::new(9.81, 5.0, 45f64.to_radians(), 0.0);
        for _ in 0..(10 * 120) {
            w.step(1.0 / 120.0, false);
        }
        let v1 = w.state().v;
        for _ in 0..(10 * 120) {
            w.step(1.0 / 120.0, false);
        }
        assert!((w.state().v - v1).abs() < 1e-9);
        assert!(w.state().s > 0.0);
    }

    #[test]
    fn para_exatamente_no_ponto_mais_alto() {
        let mut w = World::new(9.81, 5.0, 45f64.to_radians(), 30f64.to_radians());
        let mut hit = false;
        for _ in 0..(10 * 120) {
            if w.step(1.0 / 120.0, true) {
                hit = true;
                break;
            }
        }
        assert!(hit);
        let st = w.state();
        assert!((st.height - 5.0).abs() < 1e-9);
        assert!(st.v.abs() < 1e-12);
        assert!((st.s - 5.0 / 30f64.to_radians().sin()).abs() < 1e-9);
    }
}
