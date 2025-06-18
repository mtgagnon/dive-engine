SceneChangeOnSpace = { 
	OnUpdate = function(self)
		if Input.GetKeyDown("space") then
			Scene.Load("basic")
			Audio.Halt(0)
			Audio.Play(1, "dig_sfx", false)
		end
		
		
	end
}