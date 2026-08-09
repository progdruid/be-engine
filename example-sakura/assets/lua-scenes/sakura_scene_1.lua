function makeSettings()
    return {
        srm = {
            shadow     = { bias = 16.0 / 100000.0 },
            ibl        = { maxSampleRadiance = 1000.0 },
            skybox     = { clampRadiance = 0.0 },
            bloom      = { threshold = 2.5, knee = 0.7, intensity = 0.7, clamp = 4.0, upsampleRadius = 1.0 },
            tonemapper = { exposure = 0.5, contrast = 1.70 },
        },

        camera = { nearPlane = 0.1, farPlane = 250.0 },

        ambient = { color = "#000000" },

        skybox = {
            enabled = true,
            --hdrPath = "assets/moonrise_puresky.hdr",
            hdrPath = "assets/kloofendal_puresky.hdr",
        },

        bloom = { mipCount = 5, dirtTexturePath = "assets/bloom-dirt-mask.png" },

        depthOfField = { enabled = false, minFocalDistance = 0.5, focusSpeed = 5.0 },

        background = { clearColor = { 0.0 / 255.0, 23.0 / 255.0, 31.0 / 255.0 } },
    }
end

function makeShowcaseContent(data)
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

    objects.Moon = {
        transform = { position = {100, 150, 100}, scale = {6.0, 6.0, 6.0} },
        static = true,
        sunLight = {
            direction = {-1, -1.2, -1},
            color = {0.99, 0.99, 0.99},
            power = 2.0,
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

    local lightCount = 0
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
end

function makeData ()
    local data = {}
    data.Settings = makeSettings()
    data.Objects = {}

    makeShowcaseContent(data)

    return data
end
