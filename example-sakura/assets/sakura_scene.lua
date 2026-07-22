function makeScene()
    math.randomseed(os.time())

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    local scene = {}

--     scene.PBR_TestSphere = {
--         transform = { position = {8, 1, 0}, scale = {1.5, 1.5, 1.5} },
--         render = { prop = "testSphere", castShadows = true }
--     }
--     scene.PBR_TestLight = {
--         transform = { position = {8, 4, 2}, scale = {0.2, 0.2, 0.2} },
--         render = { prop = "emissiveCube", castShadows = false },
--         pointLight = { radius = 12.0, color = {1.0, 0.95, 0.85}, power = 3.0, castsShadows = false },
--         static = true
--     }
    
--     scene.Sakura2 = {
--         transform = { position = {-3.0, -5.5, 2}, rotation = {0, 45, 0}, scale = {5.0, 5.0, 5.0} },
--         render = { prop = "sakura2", castShadows = true }
--     }
--     scene.Katana = {
--         transform = { position = {10, 2.5, 0}, rotation = {30, -20, -100}, scale = {1.4, 1.4, 1.4} },
--         render = { prop = "katana", castShadows = true }
--     }

    --scene.RustySphere = {
    --    --transform = { position = {10, 2.5, -3}, scale = {0.01, 0.01, 0.01} },
    --    transform = { position = {0, 0, 0}, scale = {0.01, 0.01, 0.01} },
    --    render = { prop = "rusty-sphere", castShadows = true }
    --}

    scene.Cube = {
        transform = { position = {0, -15, 0}, scale = {30, 30, 30} },
        render = { prop = "cube", castShadows = true }
    }
    scene.Anvil = {
        transform = { position = {0, 0, 0}, scale = {0.2, 0.2, 0.2} },
        render = { prop = "anvil", castShadows = true }
    }
    scene.Phong_Axe = {
        transform = { position = {-1.5, 1.4, 1.3}, rotation = {200, 170, 0}, scale = {0.007, 0.007, 0.007} },
        render = { prop = "axe", castShadows = true }
    }
    scene.Phong_AxeLight = {
        transform = { position = {-0.5, 1.7, 1.3} },
        pointLight = { radius = 5.0, color = {1.0, 0.75, 0.2}, power = 2.0, castsShadows = false },
        static = true
    }
    scene.Katana = {
        transform = { position = { 1, 2.7, -1.04}, rotation = {180, 20, -120}, scale = {1.4, 1.4, 1.4} },
        render = { prop = "katana", castShadows = true }
    }
    scene.Compass = {
        transform = { position = {0, 5, 0}, rotation = {0, 0, 0}, scale = {10, 10, 10} },
        render = { prop = "compass", castShadows = true }
    }
    scene.KatanaLight = {
        transform = { position = { 0, 0.7, -1.8} },
        pointLight = { 
            radius = 5.0, 
            color = {255.0 / 255.0, 35.0 / 255.0, 50.0 / 255.0}, 
            power = 1.5, 
            castsShadows = false
        },
        static = true
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
            transform = { scale = { 1.0, 1.0, 1.0 } },
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
