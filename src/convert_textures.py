import numpy as np
import cv2

block_size = 16

map    = cv2.imread("sprites/block map.png")

def convert_isometric(top, left, right):
    out = np.zeros((31, 32, 3)).astype(np.uint8)

    # Transform the top
    #M = np.float32([[ 1.0, -1.0, 16 ],
    #                [ 0.5,  0.5,  0 ]])
    M = np.float32([[ 1.0, -1.0],
                    [ 0.5,  0.5]])
    M_inv = np.linalg.inv(M)

    for y in range(31):
        for x in range(32):
            if map[y,x,1] > 0:
                sample_pos = M_inv @ np.asarray([x - 16, y])

                sx = int(sample_pos[0] + .5)
                sy = int(sample_pos[1] + .5)

                if sx < 0:
                    sx = 0
                if sx > 15:
                    sx = 15

                if sy < 0:
                    sy = 0
                if sy > 15:
                    sy = 15

                out[y, x] = top[sy, sx]

    # Copy the sides into the output
    for i in range(8):
        x = 2 * i

        # Left side
        out[( 8 + i):(24 + i),       x:(x+2),   :] = left[:,x:(x+2),:]

        # Right side
        out[(15 - i):(31 - i),(16 + x):(x + 18),:] = right[:,x:(x+2),:]

    return out

def get_palette(texture_map):
    palette = [(255, 240, 192)]

    for row in texture_map:
        for pixel in row:
            r, g, b = pixel

            if not((r, g, b) in palette):
                palette.append((r, g, b))
    
    return palette

def get_closest_in_palette(palette, color):
    r, g, b = color

    rgbs = np.asarray(palette)

    dists = rgbs
    dists[:,0] -= b
    dists[:,1] -= g
    dists[:,2] -= r

    dists = np.linalg.norm(dists, axis=1)

    return np.argmin(dists)

def rearrange_palette(palette, target_colors):
    new_palette = []

    for target_color in target_colors:
        closest = palette[get_closest_in_palette(palette, target_color)]
        new_palette.append(closest)

    for color in palette:
        if color not in new_palette:
            new_palette.append(color)
    
    return new_palette

def get_palette16(palette):
    palette = np.asarray(palette).astype(np.uint16)
    full_palette = np.zeros((256, 3)).astype(np.uint16)

    # Copy the original palette into the first quarter
    full_palette[:64,:] = palette[:,:3]

    # Then add the water palette at position 128
    full_palette[128:192] = palette[:,:3]
    full_palette[128:192,0] += 255
    full_palette[128:192] //= 2

    # Then add the shaded palettes
    full_palette[64:128] = full_palette[:64] // 2
    full_palette[192:] = full_palette[128:192] // 2

    full_palette //= 8

    b = full_palette[:,0]
    g = full_palette[:,1]
    r = full_palette[:,2]

    return b + (g * 32) + (r * 1024)


def right_triangle(isometric, palette, x0, y0):
    tri = []
    y = y0
    for row in range(2, 17, 2):
        for x in range(x0, x0 + row):
            r, g, b = isometric[y, x]
            tri.append(palette.index((r, g, b)))
        y += 1
    
    for row in range(14, 1, -2):
        for x in range(x0, x0 + row):
            r, g, b = isometric[y, x]
            tri.append(palette.index((r, g, b)))
        y += 1

    return tri
    
def left_triangle(isometric, palette, x0, y0):
    tri = []
    y = y0
    for row in range(2, 17, 2):
        for x in range(x0 - row, x0):
            r, g, b = isometric[y, x]
            tri.append(palette.index((r, g, b)))
        y += 1
    
    for row in range(14, 1, -2):
        for x in range(x0 - row, x0):
            r, g, b = isometric[y, x]
            tri.append(palette.index((r, g, b)))
        y += 1

    return tri

texture_map = cv2.imread("sprites/TextureMap.png", cv2.IMREAD_UNCHANGED)

h, w, _ = texture_map.shape

# Force transparent pixels to the same color
texture_map[:,:,0][texture_map[:,:,3] == 0] = 0
texture_map[:,:,1][texture_map[:,:,3] == 0] = 0
texture_map[:,:,2][texture_map[:,:,3] == 0] = 0
texture_map = texture_map[:,:,:3]

palette = get_palette(texture_map)
palette = rearrange_palette(palette, [(192, 240, 255), (0, 0, 0), (144, 144, 144), (210, 210, 210), (255, 255, 255)])

textures = []

for x in range(0, w, block_size):
    texture = texture_map[:, x:x+block_size]

    if(np.sum(texture) == 0):
        break

    top = texture[:block_size]
    left = texture[block_size:2 * block_size]
    right = texture[2*block_size:]

    isometric = convert_isometric(top, left, right)

    texture = []
    
    texture.append(left_triangle(isometric, palette, 16, 16))
    texture.append(right_triangle(isometric, palette, 0, 8))

    texture.append(left_triangle(isometric, palette, 32, 8))
    texture.append(right_triangle(isometric, palette, 16, 16))

    texture.append(left_triangle(isometric, palette, 16, 0))
    texture.append(right_triangle(isometric, palette, 16, 0))

    textures.append(texture)

    cv2.imshow("Texture", cv2.resize(isometric, (0, 0), fx=8, fy=8, interpolation=cv2.INTER_NEAREST))
    cv2.waitKey(1)

palette16 = get_palette16(palette)

# Write the palette to the terminal
out = "\n\nuint16_t tex_palette[%d] {" % len(palette16)

for color in palette16:
    out += "%s, " % hex(color)

out += "};\n\n"

print(out)

# Write textures to the terminal
out = "\n\nTexture_t textures[%d] =\n{" % len(textures)

for texture in textures:
    out += "\n    {"

    for triangle in texture:
        out += "{"

        for color in triangle:
            out += "%s, " % hex(color)
        
        out += "}, "
    out += "},"

out += "\n};\n\n"

print(out)