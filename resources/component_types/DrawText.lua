DrawText = {

		font_name = "NotoSans-Regular",
		str_content = "hi",
		x = 0,
		y = 0,
		g = 0,
		r = 255, 
		g = 255,
		b = 255, 
		a = 255,
		font_size = 12,

	OnUpdate = function(self)
		Text.Draw(self.str_content, self.x, self.y, self.font_name, self.font_size, self.r, self.g, self.b, self.a)
	
	end
}