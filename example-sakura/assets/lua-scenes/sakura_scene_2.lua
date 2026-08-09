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
            enabled = false,
            hdrPath = "assets/moonrise_puresky.hdr",
            --hdrPath = "assets/kloofendal_puresky.hdr",
        },

        bloom = { mipCount = 5, dirtTexturePath = "assets/bloom-dirt-mask.png" },

        depthOfField = { enabled = false, minFocalDistance = 0.5, focusSpeed = 5.0 },

        background = { clearColor = { 0.0 / 255.0, 23.0 / 255.0, 31.0 / 255.0 } },
    }
end

function makeContent(data)
    local objects = data.Objects
    local settings = data.Settings

    math.randomseed(os.time())

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    objects.Cube = {
        transform = { position = {0, -15, 0}, scale = {100, 30, 100} },
        render = { prop = "cube", castShadows = true }
    }

    for i = 0, 99 do
        objects["Star_" .. i] = {
            transform = { position = { randFloat(-50, 50), randFloat(30, 60), randFloat(-50, 50) }, scale = { 0.2, 0.2, 0.2 } },
            render = { prop = "emissiveCube", castShadows = false }
        }
    end

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
            origin = {8, -8, 8},
            axis = {0, 1, 0},
            radius = 0.0,
            speed = 15.0,
            rotate = true,
        }
    }

    local lightCount = 4
    for i = 0, lightCount - 1 do
        objects["CompassLight_" .. i] = {
            transform = { scale = { 0.3, 0.3, 0.3 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = {
                radius = 25.0,
                color = {1.0, 0.95, 0.85},
                power = 3.0,
                castsShadows = true,
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

    local lightCount = 2
    for i = 0, lightCount - 1 do
        objects["CirclingLight_" .. i] = {
            transform = { scale = { 1.0, 1.0, 1.0 } },
            render = { prop = "emissiveCube", castShadows = false },
            pointLight = {
                radius = 30.0,
                color = {1.0, 0.95, 0.85},
                power = 3.0,
                castsShadows = true,
            },
            circling = {
                origin = {0.0, 8.0, 0.0},
                axis = {0, 1, 0},
                radius = 8.0,
                speed = -15.0,
                phase = 360.0 * i / lightCount,
                rotate = true,
            },
        }
    end
end

function makeData ()
    local data = {}
    data.Settings = makeSettings()
    data.Objects = {}

    makeContent(data)

    return data
end
