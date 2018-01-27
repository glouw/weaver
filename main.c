// The SDL Graphicx Primitives header is included as an additional dependency.
// (I was too lazy to software fill triangles by hand), but its still a lot
// more portable than bare-naked OpenGL.

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h> 

typedef struct
{
    float x;
    float y;
}
Point;

typedef struct
{
    Point a;
    Point b;
    Point c;
}
Tri;

typedef struct
{
    Tri* tri;
    int count;
    int max;
}
Tris;

typedef struct
{
    Point* point;
    int count;
    int max;
}
Points;

const Point zero = { 0.0, 0.0 }, one = { 1.0, 1.0 };

static Tris tsnew(const int max)
{
    const Tris ts = { (Tri*) malloc(sizeof(Tri) * max), 0, max };
    return ts;
}

static Tris tsadd(Tris tris, const Tri tri)
{
    if(tris.count == tris.max)
    {
        puts("size limitation reached");
        exit(1);
    }
    tris.tri[tris.count++] = tri;
    return tris;
}

static int peql(const Point a, const Point b)
{
    return a.x == b.x && a.y == b.y;
}

static int incircum(const Tri t, const Point p)
{
    const float ax = t.a.x - p.x;
    const float ay = t.a.y - p.y;
    const float bx = t.b.x - p.x;
    const float by = t.b.y - p.y;
    const float cx = t.c.x - p.x;
    const float cy = t.c.y - p.y;
    const float det =
        (ax * ax + ay * ay) * (bx * cy - cx * by) -
        (bx * bx + by * by) * (ax * cy - cx * ay) +
        (cx * cx + cy * cy) * (ax * by - bx * ay);
    return det > 0.0;
}

// Collects all edges from given triangles.
static Tris ecollect(Tris edges, const Tris in)
{
    for(int i = 0; i < in.count; i++)
    {
        const Tri tri = in.tri[i];
        const Tri ab = { tri.a, tri.b, zero };
        const Tri bc = { tri.b, tri.c, zero };
        const Tri ca = { tri.c, tri.a, zero };
        edges = tsadd(edges, ab);
        edges = tsadd(edges, bc);
        edges = tsadd(edges, ca);
    }
    return edges;
}

// Returns true if edge ab of two triangles are alligned.
static int alligned(const Tri a, const Tri b)
{
    return (peql(a.a, b.a) && peql(a.b, b.b)) || (peql(a.a, b.b) && peql(a.b, b.a));
}

// Flags alligned edges.
static Tris emark(Tris edges)
{
    for(int i = 0; i < edges.count; i++)
    {
        const Tri edge = edges.tri[i];
        for(int j = 0; j < edges.count; j++)
        {
            if(i == j)
                continue;
            const Tri other = edges.tri[j];
            if(alligned(edge, other))
                edges.tri[j].c = one;
        }
    }
    return edges;
}

// Creates new triangles from unique edges and appends to tris.
static Tris ejoin(Tris tris, const Tris edges, const Point p)
{
    for(int j = 0; j < edges.count; j++)
    {
        const Tri edge = edges.tri[j];
        if(peql(edge.c, zero))
        {
            const Tri tri = { edge.a, edge.b, p };
            tris = tsadd(tris, tri);
        }
    }
    return tris;
}

static SDL_Surface* load(const char* const path)
{
    SDL_Surface* const bmp = SDL_LoadBMP(path);
    SDL_PixelFormat* const allocation = SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
    SDL_Surface* const converted = SDL_ConvertSurface(bmp, allocation, 0);
    return converted;
}

// Convolution - requires a 3x3 kernel.
static uint32_t conv(uint32_t* p, const int x, const int y, const int w, const int s, const int k[3][3])
{
    return(
        (0xFF & k[0][0] * (p[(x - 1) + (y - 1) * w] >> s)) +
        (0xFF & k[0][1] * (p[(x - 0) + (y - 1) * w] >> s)) +
        (0xFF & k[0][2] * (p[(x + 1) + (y - 1) * w] >> s)) +
        (0xFF & k[1][0] * (p[(x - 1) + (y - 0) * w] >> s)) +
        (0xFF & k[1][1] * (p[(x - 0) + (y - 0) * w] >> s)) +
        (0xFF & k[1][2] * (p[(x + 1) + (y - 0) * w] >> s)) +
        (0xFF & k[2][0] * (p[(x - 1) + (y + 1) * w] >> s)) +
        (0xFF & k[2][1] * (p[(x - 0) + (y + 1) * w] >> s)) +
        (0xFF & k[2][2] * (p[(x + 1) + (y + 1) * w] >> s))) / 9.0;
}

static uint32_t* blur(uint32_t* p, const int w, const int h)
{
    const int k[3][3] = {
        { 1, 1, 1 },
        { 1, 1, 1 },
        { 1, 1, 1 },
    };
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
        out[x + y * w] =
            conv(p, x, y, w, 0x10, k) << 0x10 |
            conv(p, x, y, w, 0x08, k) << 0x08 |
            conv(p, x, y, w, 0x00, k) << 0x00;
    return out;
}

static uint32_t* grey(uint32_t* p, const int w, const int h)
{
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
    {
        const uint32_t lum =
            (0.21 * (0xFF & (p[x + y * w] >> 0x10))) +
            (0.72 * (0xFF & (p[x + y * w] >> 0x08))) +
            (0.07 * (0xFF & (p[x + y * w] >> 0x00)));
        out[x + y * w]  = lum << 0x00;
        out[x + y * w] |= lum << 0x08;
        out[x + y * w] |= lum << 0x10;
    }
    return out;
}

static uint32_t* soblx(uint32_t* p, const int w, const int h)
{
    const int k[3][3] = {
        { -1, 0, 1 },
        { -2, 0, 2 },
        { -1, 0, 1 },
    };
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
        out[x + y * w] =
            conv(p, x, y, w, 0x10, k) << 0x10 |
            conv(p, x, y, w, 0x08, k) << 0x08 |
            conv(p, x, y, w, 0x00, k) << 0x00;
    return out;
}

static uint32_t* sobly(uint32_t* p, const int w, const int h)
{
    const int k[3][3] = {
        {  1,  2,  1 },
        {  0,  0,  0 },
        { -1, -2, -1 },
    };
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
        out[x + y * w] =
            conv(p, x, y, w, 0x10, k) << 0x10 |
            conv(p, x, y, w, 0x08, k) << 0x08 |
            conv(p, x, y, w, 0x00, k) << 0x00;
    return out;
}

static uint32_t* sobl(uint32_t* p, const int w, const int h)
{
    uint32_t* sx = soblx(p, w, h);
    uint32_t* sy = sobly(p, w, h);
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
    {
        // Sobel magnitude is computed with the magnitude of each color.
        const uint32_t rx = 0xFF & (sx[x + y * w] >> 0x10);
        const uint32_t ry = 0xFF & (sy[x + y * w] >> 0x10);
        const uint32_t gx = 0xFF & (sx[x + y * w] >> 0x08);
        const uint32_t gy = 0xFF & (sy[x + y * w] >> 0x08);
        const uint32_t bx = 0xFF & (sx[x + y * w] >> 0x00);
        const uint32_t by = 0xFF & (sy[x + y * w] >> 0x00);
        out[x + y * w] =
            (uint32_t) sqrtf(rx * rx + ry * ry) << 0x10 |
            (uint32_t) sqrtf(gx * gx + gy * gy) << 0x08 |
            (uint32_t) sqrtf(bx * bx + by * by) << 0x00;
    }
    return out;
}

static uint32_t* nett(uint32_t* p, const int w, const int h, const uint32_t thresh)
{
    const int bytes = sizeof(*p) * w * h;
    uint32_t* out = memcpy((uint32_t*) malloc(bytes), p, bytes);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
    {
        const uint32_t r = 0xFF & (p[x + y * w] >> 0x10);
        const uint32_t g = 0xFF & (p[x + y * w] >> 0x08);
        const uint32_t b = 0xFF & (p[x + y * w] >> 0x00);
        out[x + y * w] =
            (r < thresh ? 0x00 : r > thresh ? 0xFF : r) << 0x10 |
            (g < thresh ? 0x00 : r > thresh ? 0xFF : g) << 0x08 |
            (b < thresh ? 0x00 : r > thresh ? 0xFF : b) << 0x00;
    }
    return out;
}

static int outob(const int x, const int y, const int w, const int h)
{
    return x < 0 || y < 0 || x >= w || y >= h;
}

static void draw(SDL_Renderer* const renderer, const int w, const int h, const Tris tris, uint32_t* regular)
{
    for(int i = 0; i < tris.count; i++)
    {
        const Tri t = tris.tri[i];
        const int x = t.a.x + (t.b.x - t.a.x) / 2.0;
        const int y = t.b.y + (t.c.y - t.b.y) / 2.0;
        const uint32_t color = outob(x, y, w, h) ? 0x00 : regular[x + y * w];
        filledTrigonColor(renderer,
                t.a.x, t.a.y, t.b.x, t.b.y, t.c.x, t.c.y,
                // Force alpha to opaque just incase its completely transparent.
                (0xFF << 24) | color);
    }
}

static void delaunay(SDL_Renderer* const renderer, const Points ps, const int w, const int h, uint32_t* regular)
{
    const int size = w * h / 3; /* "Big enough" rough approximation. */
    Tris in = tsnew(size);
    Tris out = tsnew(size);
    Tris tris = tsnew(size);
    Tris edges = tsnew(size);
    // The super triangle will snuggley fit over the screen.
    const Tri super = { { -w, 0.0 }, { 2 * w, 0.0 }, { w / 2, 2 * h } };
    tris = tsadd(tris, super);
    for(int j = 0; j < ps.count; j++)
    {
        in.count = out.count = edges.count = 0;
        const Point p = ps.point[j];
        // For all triangles...
        for(int i = 0; i < tris.count; i++)
        {
            const Tri tri = tris.tri[i];
            // Get triangles where point lies inside their circumcenter...
            if(incircum(tri, p))
                in = tsadd(in, tri);
            // And get triangles where point lies outside of their circumcenter.
            else out = tsadd(out, tri);
        }
        // Collect all triangle edges where point was inside circumcenter.
        edges = ecollect(edges, in);
        // Flag edges that are non-unique.
        edges = emark(edges);
        // Construct new triangles with unique edges.
        out = ejoin(out, edges, p);
        // Update triangle list.
        tris = out;
        // Loading bar.
        if(j % 100 == 0)
            printf("%2.0f%%\n", 100.0 * (j / (float) ps.count));
    }
    // Draw all triangles.
    draw(renderer, w, h, tris, regular);
}

static Points psnew(const int max)
{
    const Points ps = { (Point*) malloc(sizeof(Point) * max), 0, max };
    return ps;
}

static Points pcollect(uint32_t* netted, const int w, const int h, const uint32_t thresh)
{
    const int max = w * h;
    Points ps = psnew(max);
    for(int x = 1; x < w - 1; x++)
    for(int y = 1; y < h - 1; y++)
        if(netted[x + w * y] > thresh)
        {
            const Point p = { x, y };
            ps.point[ps.count++] = p;
        }
    return ps;
}

int main()
{
    const uint32_t thresh = 128;
    SDL_Surface* surface = load("geralt.bmp");
    SDL_Window* window;
    SDL_Renderer* renderer;
    const int w = surface->w;
    const int h = surface->h;
    SDL_CreateWindowAndRenderer(w, h, 0, &window, &renderer);
    // Before we get to the delaunay triangles,
    // the image is first blurred, then grey scaled, then sobel filtered for edge detection,
    // and then finally netted with a threshold [0, 255] to yield more or less triangles.
    // The pipeline is labeled with the first few letters of hte alphabet.
    uint32_t* a = (uint32_t*) surface->pixels;
    uint32_t* b = blur(a, w, h);
    uint32_t* c = grey(b, w, h);
    uint32_t* d = sobl(c, w, h);
    uint32_t* e = nett(d, w, h, thresh);
    const Points ps = pcollect(e, w, h, thresh);
    // Note that the original image is used for coloring delaunay triangles.
    delaunay(renderer, ps, w, h, a);
    SDL_RenderPresent(renderer);
    SDL_Delay(10000);
    // No need to free hoisted memory - gives a fast exit.
}