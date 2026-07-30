function makeSettings()
    return {
        srm = {
            shadow     = { bias = 16.0 / 100000.0 },
            ibl        = { maxSampleRadiance = 100.0 },
            skybox     = { clampRadiance = 0.0 },
            bloom      = { threshold = 2.5, knee = 0.7, intensity = 0.7, clamp = 4.0, upsampleRadius = 1.0 },
            tonemapper = { exposure = 0.25, contrast = 1.70 },
        },

        camera = { nearPlane = 0.1, farPlane = 250.0 },

        ambient = { color = "#000000" },

        skybox = {
            enabled = true,
            hdrPath = "assets/moonrise_puresky.hdr",
            --hdrPath = "assets/kloofendal_puresky.hdr",
        },

        bloom = { mipCount = 5, dirtTexturePath = "assets/bloom-dirt-mask.png" },

        depthOfField = { enabled = false, minFocalDistance = 0.5, focusSpeed = 5.0 },

        background = { clearColor = { 0.0 / 255.0, 23.0 / 255.0, 31.0 / 255.0 } },
    }
end


function makeBase(data)
    local objects = data.Objects
    local settings = data.Settings

    math.randomseed(os.time())

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    objects.Cube = {
        transform = { position = {0, -15, 0}, scale = {30, 30, 30} },
        render = { prop = "cube", castShadows = true }
    }

    for i = 0, 99 do
        objects["Star_" .. i] = {
            transform = { position = { randFloat(-50, 50), randFloat(30, 60), randFloat(-50, 50) }, scale = { 0.2, 0.2, 0.2 } },
            render = { prop = "emissiveCube", castShadows = false }
        }
    end
end


function makeStandardContent (data)
    local objects = data.Objects
    local settings = data.Settings

    objects.Moon = {
        transform = { position = {100, 150, 100}, scale = {6.0, 6.0, 6.0} },
        render = { prop = "moon", castShadows = false },
        static = true,
        sunLight = {
            direction = {-1, -1, -1},
            color = {0.7, 0.7, 0.99},
            power = 0.0,
            castsShadows = true,
            shadowMapResolution = 4096,
            shadowCameraDistance = 100.0,
            shadowMapWorldSize = 100.0,
            shadowNearPlane = 0.1,
            shadowFarPlane = 400.0
        }
    }   
    
    --objects.RustySphere = {
    --    --transform = { position = {10, 2.5, -3}, scale = {0.01, 0.01, 0.01} },
    --    transform = { position = {0, 0, 0}, scale = {0.01, 0.01, 0.01} },
    --    render = { prop = "rusty-sphere", castShadows = true }
    --}
    objects.Anvil = {
        transform = { position = {0, 0, 0}, scale = {0.2, 0.2, 0.2} },
        render = { prop = "anvil", castShadows = true }
    }
    objects.Phong_Axe = {
        transform = { position = {-1.5, 1.4, 1.3}, rotation = {200, 170, 0}, scale = {0.007, 0.007, 0.007} },
        render = { prop = "axe", castShadows = true }
    }
    objects.Phong_AxeLight = {
        transform = { position = {-0.5, 1.7, 1.3} },
        pointLight = { radius = 5.0, color = {1.0, 0.75, 0.2}, power = 2.0, castsShadows = false },
        static = true
    }
    objects.Katana = {
        transform = { position = { 1, 2.7, -1.04}, rotation = {180, 20, -120}, scale = {1.4, 1.4, 1.4} },
        render = { prop = "katana", castShadows = true }
    }
    objects.KatanaLight = {
        transform = { position = { 0, 0.7, -1.8} },
        pointLight = { 
            radius = 5.0, 
            color = {255.0 / 255.0, 35.0 / 255.0, 50.0 / 255.0}, 
            power = 1.5, 
            castsShadows = false
        },
        static = true
    }

    local lightCount = 4
    for i = 0, lightCount - 1 do
        objects["PointLight_" .. i] = {
            transform = { scale = { 1.0, 1.0, 1.0 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = {
                radius = 15.0,
                color = {1.0, 0.95, 0.85},
                power = 3.0,
                castsShadows = false,
            },
            circling = {
                origin = {0.0, 4.0 + 4.0 * (i % 2), 0.0},
                axis = {0, 1, 0},
                radius = 13.0 * (0.7 + 0.3 * ((i + 1) % 2)),
                speed = 15.0,
                phase = 360.0 * i / lightCount,
            },
        }
    end
end


function makeShowcaseContent(data)
    local objects = data.Objects
    local settings = data.Settings

    objects.Moon = {
        transform = { position = {100, 150, 100}, scale = {6.0, 6.0, 6.0} },
        render = { prop = "moon", castShadows = false },
        static = true,
        sunLight = {
            direction = {-1, -1, -1},
            color = {0.7, 0.7, 0.99},
            power = 0.0,
            castsShadows = true,
            shadowMapResolution = 4096,
            shadowCameraDistance = 100.0,
            shadowMapWorldSize = 100.0,
            shadowNearPlane = 0.1,
            shadowFarPlane = 400.0
        }
    }
    
    objects.Compass = {
        transform = { rotation = {0, 0, 0}, scale = {30, 30, 30} },
        render = { prop = "compass", castShadows = true },
        circling = {
            origin = {0, -8, 0},
            axis = {0, 1, 0},
            radius = 0.0,
            speed = 15.0,
            rotate = true,
        }
    }

    --objects.Book = {
    --    transform = { rotation = {0, 0, 0}, scale = {3, 3, 3} },
    --    render = { prop = "book", castShadows = true },
    --    circling = {
    --        origin = {0, 0, 0},
    --        axis = {0, 1, 0},
    --        radius = 0.0,
    --        speed = 15.0,
    --        rotate = true,
    --    }
    --}

    local lightCount = 4
    for i = 0, lightCount - 1 do
        objects["CompassLight_" .. i] = {
            transform = { scale = { 0.3, 0.3, 0.3 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = {
                radius = 15.0,
                color = {1.0, 0.95, 0.85},
                power = 3.0,
                castsShadows = false,
            },
            circling = {
                origin = {0.0, 4.0, 0.0},
                axis = {0, 1, 0},
                radius = 0.8,
                speed = 30.0,
                phase = 360.0 * i / lightCount,
            },
        }
    end

    local lightCount = 4
    for i = 0, lightCount - 1 do
        objects["CirclingLight_" .. i] = {
            transform = { scale = { 1.0, 1.0, 1.0 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = {
                radius = 20.0,
                color = {1.0, 0.95, 0.85},
                power = 3.0,
                castsShadows = false,
            },
            circling = {
                origin = {0.0, 2.0, 0.0},
                axis = {0, 1, 1},
                radius = 8.0,
                speed = -15.0,
                phase = 360.0 * i / lightCount,
            },
        }
    end
end

function makeSpinningLights(data)
    local objects = data.Objects
    local settings = data.Settings

    settings.skybox.enabled = false
    settings.bloom.mipCount = 4

    objects.Cube.transform.scale = {60, 30, 60}

    local clusterCount = 3
    local clusterSpacing = 10.0
    local lightCount = 4
    local saturation = 0.8

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    local function hueColor(hue)
        local function channel(offset)
            local value = math.abs(((hue + offset) % 1.0) * 6.0 - 3.0) - 1.0
            return math.max(0.0, math.min(1.0, value))
        end
        local r, g, b = channel(0.0), channel(2.0 / 3.0), channel(1.0 / 3.0)
        return {
            1.0 - saturation + saturation * r,
            1.0 - saturation + saturation * g,
            1.0 - saturation + saturation * b,
        }
    end

    for cx = 0, clusterCount - 1 do
        for cz = 0, clusterCount - 1 do
            local cluster = cx * clusterCount + cz
            local originX = (cx - (clusterCount - 1) * 0.5) * clusterSpacing + randFloat(-2.0, 2.0)
            local originZ = (cz - (clusterCount - 1) * 0.5) * clusterSpacing + randFloat(-2.0, 2.0)
            local yaw = math.random() * 2.0 * math.pi
            local phaseOffset = math.random() * 360.0
            local color = hueColor(cluster / (clusterCount * clusterCount) + randFloat(-0.03, 0.03))
            local orbitRadius = randFloat(5.0, 8.0)
            local speed = randFloat(10.0, 20.0) * (cluster % 2 == 0 and 1.0 or -1.0)

            for i = 0, lightCount - 1 do
                objects["CirclingLight_" .. cx .. "_" .. cz .. "_" .. i] = {
                    transform = { scale = { 0.1, 0.1, 0.1 } },
                    render = { prop = "emissiveCube", castShadows = false },
                    pointLight = {
                        radius = 20.0,
                        color = color,
                        power = 4.0,
                        castsShadows = false,
                    },
                    circling = {
                        origin = { originX, 0.0, originZ },
                        axis = { math.sin(yaw), 0, math.cos(yaw) },
                        radius = orbitRadius,
                        speed = speed,
                        phase = phaseOffset + 360.0 * i / lightCount,
                    },
                }
            end
        end
    end
end

function makeData ()
    local data = {}
    data.Settings = makeSettings()
    data.Objects = {}

    makeBase(data)
    --makeStandardContent(data)
    --makeShowcaseContent(data)
    makeSpinningLights(data)

    return data
end
