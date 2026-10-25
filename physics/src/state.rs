/// Layout idêntico ao de PhysicsState em cpp/physics/rust_api.h
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct PhysicsState {
    pub s: f64,
    pub v: f64,
    pub x: f64,
    pub y: f64,
    pub tx: f64, // tangente unitária (direção de s crescente)
    pub ty: f64,
    pub speed: f64,
    pub height: f64,
    pub energy: f64, // por unidade de massa: v²/2 + g*h
    pub time: f64,
    pub steps: u64, // passos de física executados desde o reset
    pub apex: u32,  // 1 = parou no ponto mais alto do plano 2 (simulação congelada)
    pub _pad: u32,
}
