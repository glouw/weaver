
const Tri t = tris.tri[i];
const int x = t.a.x + (t.b.x - t.a.x) / 2.0;
const int y = t.b.y + (t.c.y - t.b.y) / 2.0;
const uint32_t color = regular[x + y * w];
filledTrigonColor(renderer, t.a.x, t.a.y, t.b.x, t.b.y, t.c.x, t.c.y, (0xFF << 24) | color);
