-- Sprite Atlas Loader Demo
-- This demo shows how to load sprite atlas from CSV and PNG file
-- Atlas generated using Mugen Sprite Viewer
-- https://github.com/leonkasovan/MugenSpriteViewer
-- 12 Mei 2025 (c) leonkasovan@gmail.com

local g_players = {}
local g_loaded_atlas_img = {}
local gw, gh = love.graphics.getDimensions()
local is_playing = true

local MAX_SPRITE = 8
local MAX_SPRITE_PER_ROW = 4
local MAX_SPRITE_PER_COL = 2
local TICK_LIMIT = 0.08 -- normal

function loadAtlasImage(filename)
	for k, v in ipairs(g_loaded_atlas_img) do
		if v.filename == filename then
			return v.image
		end
	end
	local i = {}
	i.filename = filename
	i.image = love.graphics.newImage(filename)
	table.insert(g_loaded_atlas_img, i)
	return i.image
end

function loadChar(name, x, y)
	local player = {}
	player.name = string.match(name, "sprite_atlas_(.+)")
	player.atlas_img = loadAtlasImage("sprite_atlas_" .. player.name .. ".png")
	player.atlas_dat = {}
	for line in io.lines("sprite_atlas_" .. player.name .. ".csv") do
		if #line > 0 then
			local spr_x, spr_y, spr_w, spr_h, spr_off_x, spr_off_y, spr_group_id, spr_img_no =
				line:match(
					"([%d%-]+),([%d%-]+),(%d+),(%d+),([%d%-]+),([%d%-]+),(%d+),(%d+)")

			if spr_x == nil or spr_img_no == nil then
				print("Skip: ", line)
			else
				table.insert(player.atlas_dat, {
					tonumber(spr_x), tonumber(spr_y), tonumber(spr_w), tonumber(spr_h),
					tonumber(spr_off_x), tonumber(spr_off_y), tonumber(spr_group_id), tonumber(spr_img_no)
				})
			end
		end
	end
	player.atlas_img_w, player.atlas_img_h = player.atlas_img:getDimensions()
	player.x = x
	player.y = y
	player.tick = 0
	player.frame_no = 1
	return player
end

function love.load()
	local sprites = {}
	local spr_x, delta_x
	local spr_y, delta_y

	love.window.setVSync(1)
	love.window.setTitle("Sprite Atlas Loader Demo")

	local files = love.filesystem.getDirectoryItems("")
	local n_csv = 0
	for _, file in ipairs(files) do
		if file:lower():sub(-4) == ".csv" then
			table.insert(sprites, file:match("^(.*)%.%w+$") or file)
			n_csv = n_csv + 1
		end
		if n_csv >= MAX_SPRITE then
			break
		end
	end

	if #sprites <= MAX_SPRITE_PER_ROW then
		delta_x = gw / (#sprites + 1)
	else
		delta_x = gw / (MAX_SPRITE_PER_ROW + 1)
	end
	delta_y = gh / (math.floor((#sprites - 1) / MAX_SPRITE_PER_ROW) + 2)

	for k, spr in ipairs(sprites) do
		spr_x = ((k - 1) % MAX_SPRITE_PER_ROW + 1) * delta_x
		spr_y = (math.floor((k - 1) / MAX_SPRITE_PER_ROW) + 1) * delta_y
		table.insert(g_players, loadChar(spr, spr_x, spr_y))
	end
end

function love.keypressed(key)
	if key == "space" then
		is_playing = not is_playing
	elseif key == "left" then
		for _, player in ipairs(g_players) do
			player.frame_no = player.frame_no - 1
			if player.frame_no < 1 then
				player.frame_no = 1
			end
		end
	elseif key == "right" then
		for _, player in ipairs(g_players) do
			player.frame_no = player.frame_no + 1
			if player.frame_no > #player.atlas_dat then
				player.frame_no = #player.atlas_dat
			end
		end
	elseif key == "home" then
		for _, player in ipairs(g_players) do
			player.frame_no = 1
		end
	elseif key == "end" then
		for _, player in ipairs(g_players) do
			player.frame_no = #player.atlas_dat
		end
	elseif key == "pageup" then
		for _, player in ipairs(g_players) do
			player.frame_no = player.frame_no - 10
			if player.frame_no < 1 then
				player.frame_no = 1
			end
		end
	elseif key == "pagedown" then
		for _, player in ipairs(g_players) do
			player.frame_no = player.frame_no + 10
			if player.frame_no > #player.atlas_dat then
				player.frame_no = #player.atlas_dat
			end
		end
	elseif key == "up" then
		TICK_LIMIT = TICK_LIMIT - 0.01
		if TICK_LIMIT < 0.01 then
			TICK_LIMIT = 0.01
		end
	elseif key == "down" then
		TICK_LIMIT = TICK_LIMIT + 0.01
	elseif key == "escape" then
		love.event.quit()
	end
end

function love.update(dt)
	if is_playing then
		for _, player in ipairs(g_players) do
			player.tick = player.tick + dt
			if player.tick > TICK_LIMIT then
				player.tick = 0
				player.frame_no = player.frame_no + 1
				if player.frame_no > #player.atlas_dat then
					player.frame_no = 1 -- loop
				end
			end
		end
	end
end

function love.draw()
	for i, player in ipairs(g_players) do
		local data = player.atlas_dat[player.frame_no]
		if data then
			love.graphics.draw(player.atlas_img,
				love.graphics.newQuad(data[1], data[2], data[3], data[4], player.atlas_img_w, player.atlas_img_h),
				player.x - data[5], player.y - data[6])
			love.graphics.printf(
				string.format("[%d] %s\nTotal Sprite: %d\nCurrent [%d]: %d,%d", i, player.name, #player.atlas_dat,
					player.frame_no, data[7], data[8]), player.x - 50, player.y + 5, 200, "left")
		else
			love.graphics.printf(string.format("P%d: Invalid frame", i), player.x, player.y + 5, 200, "left")
		end
	end
	local stats = love.graphics.getStats()
	love.graphics.setColor(1, 1, 0, 1)
	love.graphics.print("Sprite Atlas Loader Demo", 10, 10)
	love.graphics.setColor(0, 1, 0, 1)
	love.graphics.print("Press ESC to quit", 10, 25)
	love.graphics.print("Press SPACE to pause", 10, 40)
	love.graphics.print("Press UP/DOWN to change speed", 10, 55)
	love.graphics.print("Press LEFT/RIGHT to change frame", 10, 70)
	love.graphics.print("Speed: " .. tostring(TICK_LIMIT), 10, gh - 65)
	love.graphics.print("FPS: " .. tostring(love.timer.getFPS()), 10, gh - 50)
	love.graphics.print("Draw Calls: " .. stats.drawcalls, 10, gh - 35)
	love.graphics.print("Texture Memory: " .. tostring(math.floor(stats.texturememory / 1024 / 1024)) .. " MB", 10,
		gh - 20)
	-- reset color
	love.graphics.setColor(1, 1, 1, 1)
end
