Generator = {
	spawn_rate = 60*5,

	OnUpdate = function(self)
		if Application.GetFrame() % self.spawn_rate == 0 then
			local new_strawberry = Actor.Instantiate("Strawberry")
			local new_strawberry_rb = new_strawberry:GetComponent("Rigidbody")
			new_strawberry_rb.x = 19
			new_strawberry_rb.y = 10
		end
	end
}

