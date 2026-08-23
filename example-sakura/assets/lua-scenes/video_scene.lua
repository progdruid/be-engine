function makeData()
    local data = {}

    data.Settings = {
        srm = {
            shadow     = { bias = 16.0 / 100000.0 },
            bloom      = { threshold = 1.5, knee = 0.7, intensity = 0.5, clamp = 4.0, upsampleRadius = 1.0 },
            tonemapper = { exposure = 0.5, contrast = 1.1 },
        },

        camera = { nearPlane = 0.1, farPlane = 250.0 },

        ambient = { color = "#1a2233" },

        backbuffer = { backgroundColor = '#151515', discardFar = true },
        --backbuffer = { backgroundColor = '#000000', discardFar = true },
    }

    data.Objects = {}

    data.Objects.Plane = {
        transform = { position = { 0, 0, 0 }, scale = { 40, 1, 40 } },
        render = { prop = "displaced-plane", castShadows = false },
    }

    data.Objects.Sun = {
        transform = { position = { 0, 0, 0 } },
        static = true,
        sunLight = {
            direction = { -0.5, -1.0, -0.35 },
            color = { 1.0, 0.96, 0.88 },
            power = 3.0,
            castsShadows = false,
        },
    }

    return data
end
