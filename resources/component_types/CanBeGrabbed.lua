CanBeGrabbed = {
	grabbed_by = nil,
	index = 0,

	OnUpdate = function(self)
		local rb = self.actor:GetComponent("Rigidbody")
		if self.grabbed_by ~= nil then 
			local other_rb = self.grabbed_by:GetComponent("Rigidbody")
			rb:SetVelocity(Vector2(other_rb:GetVelocity().x, rb:GetVelocity().y))
		end
	
	end,

	OnTriggerEnter = function(self, collision)
        local rb = self.actor:GetComponent("Rigidbody")
        local actor_rb = nil

        if collision.other:GetName() == "player" or collision.other:GetName() == "player2" then
            self.grabbed_by = collision.other
            actor_rb = collision:GetComponent("Rigidbody")
        end

        if actor_rb ~= nil then 
			Debug.Log("setting strawberry pos")
            rb:SetPosition(actor_rb:GetPosition() - Vector2(5,10))
        end
	end
}

