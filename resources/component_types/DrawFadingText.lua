DrawFadingText = {

		font_name = "NotoSans-Regular",
		str_content = "hi",
		x = 0,
		y = 0,
		r = 255, 
		g = 255,
		b = 255, 
		a = 255,
		font_size = 12,
		fade_goal = 255,

	OnUpdate = function(self)
		self.a = math.ceil(self.a * (1 - 0.05) + self.fade_goal * 0.05)
		if self.a <= 20 then
			self.fade_goal = 255
		elseif self.a >= 254 then 
			self.fade_goal = 0
		
		end

		Text.Draw(self.str_content, self.x, self.y, self.font_name, self.font_size, self.r, self.g, self.b, self.a)
	end
}