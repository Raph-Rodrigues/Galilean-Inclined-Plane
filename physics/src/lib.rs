//! Física do plano inclinado de Galileu.
//!
//! - `world`:  física pura (uma partícula em dois planos), sem threads
//! - `engine`: thread de física a passo fixo + snapshots
//! - `ffi`:    API extern "C" consumida pelo C++
//! - `state`:  struct #[repr(C)] compartilhada com o C++
#![allow(improper_ctypes_definitions)] // handles opacos: o C++ nunca os desreferencia

mod engine;
mod ffi;
mod state;
mod world;

pub use engine::Engine;
pub use ffi::*;
pub use state::PhysicsState;
