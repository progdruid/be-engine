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
    }
end

function makeSpinningLights(data)
    local objects = data.Objects
    local settings = data.Settings

    math.randomseed(os.time())

    local function randFloat(min, max)
        return min + math.random() * (max - min)
    end

    objects.Cube = {
        transform = { position = {0, -15, 0}, scale = {120, 30, 120} },
        render = { prop = "cube", castShadows = true }
    }
    objects.Cube2 = {
        transform = { position = {0, 30, 0}, scale = {120, 30, 120} },
        render = { prop = "cube", castShadows = true }
    } 
    
    
    settings.skybox.enabled = false
    settings.bloom.mipCount = 4

    local clusterCount = 10
    local clusterSpacing = 7.0
    local lightCount = 5
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
            local orbitRadius = randFloat(8.0, 10.0)
            local speed = randFloat(10.0, 20.0) * (cluster % 2 == 0 and 1.0 or -1.0)

            for i = 0, lightCount - 1 do
                objects["CirclingLight_" .. cx .. "_" .. cz .. "_" .. i] = {
                    transform = { scale = { 0.1, 0.1, 0.1 } },
                    render = { prop = "emissiveCube", castShadows = false },
                    pointLight = {
                        radius = 15.0,
                        color = color,
                        power = 4.0,
                        castsShadows = false,
                    },
                    circling = {
                        origin = { originX, 5.0, originZ },
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

    makeSpinningLights(data)

    return data
end
