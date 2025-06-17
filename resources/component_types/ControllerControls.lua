ControllerControls = {
	speed = 5,
	jump_power = 350,
	controller_id = 0,

	OnStart = function(self)
		self.rb = self.actor:GetComponent("Rigidbody")
	end,

	OnUpdate = function(self)
		-- Horizontal
		local horizontal_input = 0
		Debug.Log(self.controller_id)
		horizontal_input = self.speed*Input.GetControllerAxis(self.controller_id, "leftx")

		-- Vertical
		local vertical_input = 0
		vertical_input = self.speed*Input.GetControllerAxis(self.controller_id, "lefty")

		self.rb:SetVelocity(Vector2(horizontal_input, vertical_input))
	end
}

