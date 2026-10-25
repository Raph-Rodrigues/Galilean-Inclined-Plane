//! API C. Só traduz ponteiros crus em chamadas ao `Engine`.
//!
//! Segurança: o C++ deve passar ponteiros obtidos de `physics_create` e não
//! usá-los depois de `physics_destroy`.

use crate::engine::Engine;
use crate::state::PhysicsState;

#[no_mangle]
pub extern "C" fn physics_create(
    g: f64,
    h0: f64,
    theta1: f64,
    theta2: f64,
    dt: f64,
) -> *mut Engine {
    match Engine::spawn(g, h0, theta1, theta2, dt) {
        Some(e) => Box::into_raw(Box::new(e)),
        None => std::ptr::null_mut(),
    }
}

/// Para a thread de física, espera ela terminar e libera o handle.
#[no_mangle]
pub unsafe extern "C" fn physics_destroy(e: *mut Engine) {
    if !e.is_null() {
        drop(Box::from_raw(e)); // Drop do Engine faz o join
    }
}

#[no_mangle]
pub unsafe extern "C" fn physics_reset(e: *mut Engine, g: f64, h0: f64, theta1: f64, theta2: f64) {
    if let Some(e) = e.as_ref() {
        e.reset(g, h0, theta1, theta2);
    }
}

/// Copia os dois últimos snapshots e o fator de interpolação (0..1).
#[no_mangle]
pub unsafe extern "C" fn physics_get_snapshots(
    e: *const Engine,
    prev: *mut PhysicsState,
    curr: *mut PhysicsState,
    alpha: *mut f64,
) {
    let (Some(e), Some(prev), Some(curr), Some(alpha)) =
        (e.as_ref(), prev.as_mut(), curr.as_mut(), alpha.as_mut())
    else {
        return;
    };
    let (p, c, a) = e.snapshots();
    *prev = p;
    *curr = c;
    *alpha = a;
}

#[no_mangle]
pub unsafe extern "C" fn physics_set_paused(e: *mut Engine, paused: bool) {
    if let Some(e) = e.as_ref() {
        e.set_paused(paused);
    }
}

#[no_mangle]
pub unsafe extern "C" fn physics_set_speed(e: *mut Engine, speed: f64) {
    if let Some(e) = e.as_ref() {
        e.set_speed(speed);
    }
}
