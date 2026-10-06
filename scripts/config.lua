return {
	title = "Plano Inclinado de Galileu",
	width = 1280,
	height = 720,

	physics = {
		g = 9.81, -- m/s²
		h0 = 5.0, -- altura inicial da bola (m)
		theta1 = 45.0, -- angulo do plano 1 em graus
		theta2 = 30.0, -- angulo inicial do plano 2 em graus
	},

	ball = {
		radius = 0.3, -- m
		color = { 255, 153, 51 }, -- RGB
	},

	view = {
		scale = 40.0, -- px/m
		vel_scale = 0.5, -- metros de seta por (m/s)
		max_run = 15.0, -- comprimento maximo desenhado do plano 2 (m)
	},

	-- etapa 5 -> sequencia de situações
	angles = { 60, 45, 30, 15, 5, 0 },
}
