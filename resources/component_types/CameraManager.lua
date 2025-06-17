CameraManager = {
	ease_factor = 0.1,
	tracking_player = true,
	set_pos = false,

	OnUpdate = function(self)
		-- local player_actor = Actor.Find("player")
		-- if player_actor == nil then
		-- 	self.tracking_player = false
		-- 	Debug.Log("no player")
		-- 	return
		
		-- elseif self.tracking_player == true then
		-- 	self.set_pos = true
		-- 	local rb = player_actor:GetComponent("Rigidbody")
		-- 	local player_pos = rb:GetPosition()
		-- 	Camera.SetPosition(player_pos.x, player_pos.y)
		-- 	return
		-- end
	end
}

