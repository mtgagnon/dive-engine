Scene1 = {

	-- TILE CODES --
	-- 0 : nothing
	-- 1 : grass
	-- 2 : player
	-- 3 : strawberry
	-- 4 : player2
	-- 5 : dirt
	-- 6 : rock

	OnStart = function(self)
		local controller = 0
		local new_ant_hill = Actor.Instantiate("Ant_Hill")
		new_ant_hill.x = 2
		new_ant_hill.y = 1
		-- Spawn stage
		
	end,

	OnUpdate = function(self)
		
	end
}

