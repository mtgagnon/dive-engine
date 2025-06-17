DrawScore = {

		font_name = "NotoSans-Regular",
		str_content = "hi",
		x = 100,
		y = 50,
		g = 0,
		r = 0, 
		g = 0,
		b = 0, 
		a = 0,
		font_size = 40,
		score = 0,

	OnUpdate = function(self)
		if self.str_content == "Wins!" then
			
		else 
			self.str_content = "Score: " .. self.score
		end
		Text.Draw(self.str_content, self.x, self.y, self.font_name, self.font_size, self.r, self.g, self.b, self.a)
	
	end
}