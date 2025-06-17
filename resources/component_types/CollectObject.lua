CollectObject = {
	heldObject = nil,
	InRange = false,
	strawberryActor = nil,
	textObject = nil,
	sprite = "",

	OnStart = function(self)
		self.textObject = self.actor:GetComponent("DrawScore")
		self.sprite = self.actor:GetComponent("DrawUI").sprite
	end,

	OnUpdate = function(self)
		if Input.GetKeyDown("e") and self.InRange then
		end
	end,

	OnTriggerEnter = function(self, collision)
		if collision.other:GetName() == "Strawberry" then
			self.InRange = true
			Debug.Log("AntHill InRange: " .. tostring(self.InRange))
			self.strawberryActor = collision.other
			Actor.Destroy(self.strawberryActor)
			self.textObject.score = self.textObject.score + 1

		end
	end,

	OnTriggerExit = function(self, collision)
	if collision.other:GetName() == "Strawberry" then
		self.InRange = false
		Debug.Log("InRange: " .. tostring(self.InRange))
	end
	end

}
