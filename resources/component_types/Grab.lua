Grab = {
	heldObject = nil,
	InRange = false,
	strawberryActor = nil,

	OnStart = function(self)
	end,

	OnUpdate = function(self)
		if Input.GetKeyDown("e") and self.InRange then
		end
	end,

	OnTriggerEnter = function(self, collision)
		if collision.other:GetName() == "Strawberry" then
			self.InRange = true
			Debug.Log("InRange: " .. tostring(self.InRange))
			self.strawberryActor = collision.other
			-- Actor.Destroy(collision.other)
		end
	end,

	OnTriggerExit = function(self, collision)
	if collision.other:GetName() == "Strawberry" then
		self.InRange = false
		Debug.Log("InRange: " .. tostring(self.InRange))
	end
	end

}

