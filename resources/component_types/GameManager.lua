GameManager = {

	-- TILE CODES --
	-- -1 : dirt
	-- 0 : nothing
	-- 1 : Static box
	-- 2 : player
	-- 3 : strawberry
	-- 4 : player2
	-- 5 : stem
	-- 6 : camera_center
	-- 7 : ant_hill
	-- 8 : left leaf
	-- 9 : right leaf

	
	stage1 = {
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, --21x37
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
		{1, 0, 0, 7, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 1},
		{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
		{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	},

	OnStart = function(self)
		local controller = 0
		local center_pos
		-- Spawn stage
		for y=1,21 do 
			for x = 1,37 do
				local tile_code = self.stage1[y][x]
				local tile_pos = Vector2(x, y)


				if tile_code == -1 then 
					local new_tile = Actor.Instantiate("Dirt_Tile")
					local new_tile_rb = new_tile:GetComponent("Rigidbody")

					new_tile_rb.x = tile_pos.x
					new_tile_rb.y = tile_pos.y
				elseif tile_code == 1 then 
					local new_tile = Actor.Instantiate("Ground_Tile")
					local new_tile_rb = new_tile:GetComponent("Rigidbody")
					if y == 20 and x ~= 19 then
						new_tile_rb.has_collider = false
					elseif y == 20 then
						new_tile_rb.width = 37
						local tile_pos = new_tile_rb:GetPosition()
						Camera.SetPosition(tile_pos.x, tile_pos.y)
					end
		

					new_tile_rb.x = tile_pos.x
					new_tile_rb.y = tile_pos.y
				elseif tile_code == 2 then
					Debug.Log("instantiating player")

					local new_player = Actor.Instantiate("player")
					local new_player_rb = new_player:GetComponent("Rigidbody")
					-- CONTROLLER API
					-- local new_player_controls = new_player:GetComponent("ControllerControls")
					-- new_player_controls.controller_id = controller
					-- controller = controller+1

					new_player_rb.x = tile_pos.x
					new_player_rb.y = tile_pos.y
				elseif tile_code == 3 then
					local new_strawberry = Actor.Instantiate("Strawberry")
					local new_strawberry_rb = new_strawberry:GetComponent("Rigidbody")
					new_strawberry_rb.x = tile_pos.x
					new_strawberry_rb.y = tile_pos.y
				elseif tile_code == 4 then
					Debug.Log("instantiating player 2")
					local new_player2 = Actor.Instantiate("player2")
					local new_player2_rb = new_player2:GetComponent("Rigidbody")
					new_player2_rb.x = tile_pos.x
					new_player2_rb.y = tile_pos.y

				elseif tile_code == 5 then
					local new_tile = Actor.Instantiate("Stem")
					local new_tile = new_tile:GetComponent("Rigidbody")
					new_tile.x = tile_pos.x
					new_tile.y = tile_pos.y
				elseif tile_code == 6 then 
					center_pos = tile_pos
				elseif tile_code == 7 then
					Debug.Log("ant_hill 2")
					local new_player2 = Actor.Instantiate("Ant_Hill")
					local new_player2_rb = new_player2:GetComponent("Rigidbody")
					local new_player2_text = new_player2:GetComponent("DrawScore")
					local new_player2_sprite = new_player2:GetComponent("DrawUI")
					if tile_pos.x > 6 then
						new_player2_text.x = 1200
						new_player2_sprite.sprite = "ant2"
						new_player2_sprite.x = 1200
					else 
						new_player2_text.x = 100
						new_player2_sprite.sprite = "ant"
						new_player2_sprite.x = 100
					end
					new_player2_rb.x = tile_pos.x
					new_player2_rb.y = tile_pos.y
				elseif tile_code == 8 then
					local new_tile = Actor.Instantiate("Leaf_left")
					local new_tile = new_tile:GetComponent("Rigidbody")
					new_tile.x = tile_pos.x
					new_tile.y = tile_pos.y
				elseif tile_code == 9 then
					local new_tile = Actor.Instantiate("Leaf_right")
					local new_tile = new_tile:GetComponent("Rigidbody")
					new_tile.x = tile_pos.x
					new_tile.y = tile_pos.y
				end
		
			end
		end

		Camera.SetPosition(center_pos.x, center_pos.y)
		Camera.SetZoom(0.5)

	end,

	OnUpdate = function(self)
		
	end
}

