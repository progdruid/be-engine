function sakuraDefaults()
	local defaults = {}

	defaults.circlingLightsOrigin = {0.0, 6.0, 3.0}
	defaults.circlingLightsAddY = 0
	defaults.circlingLightsRadius = 1

	defaults.orbitCameraOrigin = {0.0, 0.0, 3.0}

	return defaults
end

function makeScene()
    math.randomseed(os.time())

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    local scene = {}

    scene.Cube = {
        transform = { position = {0, -15, 0}, scale = {30, 30, 30} },
        render = { prop = "cube", castShadows = true }
    }
    scene.Compass = {
        transform = { position = {0, -8, 3}, rotation = {0, 0, 0}, scale = {30, 30, 30} },
        render = { prop = "compass", castShadows = true }
    }

    scene.Moon = {
        transform = { position = {100, 150, 100}, scale = {6.0, 6.0, 6.0} },
        render = { prop = "moon", castShadows = false },
        static = true,
        sunLight = {
            direction = {-1, -1, -1},
            color = {0.7, 0.7, 0.99},
            power = 2.0,
            castsShadows = true,
            shadowMapResolution = 4096,
            shadowCameraDistance = 100.0,
            shadowMapWorldSize = 100.0,
            shadowNearPlane = 0.1,
            shadowFarPlane = 400.0
        }
    }   

    for i = 0, 99 do
        scene["Star_" .. i] = {
            transform = { position = { randFloat(-50, 50), randFloat(30, 60), randFloat(-50, 50) }, scale = { 0.2, 0.2, 0.2 } },
            render = { prop = "emissiveCube", castShadows = false }
        }
    end

    for i = 0, 3 do
        scene["PointLight_" .. i] = {
            transform = { scale = { 0.2, 0.2, 0.2 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = { 
                radius = 15.0, 
                color = {1.0, 0.95, 0.85}, 
                power = 3.0, 
                castsShadows = false,
            },
        }
    end

    return scene
end
