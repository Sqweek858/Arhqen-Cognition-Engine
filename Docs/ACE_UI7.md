# ACE-UI7 - Draw Command Buffer

ACE-UI7 adds a lightweight draw command buffer. The goal is to make the render path inspectable, cacheable and eventually sortable by layer, like a simplified draw-element stream.

Implemented:

- `D2DDrawCommandBuffer`.
- Commands for clip, rect, rounded rect, text and line.
- Per-frame command counters.
- Layer field for future sorting/debugging.
- Shell begins a command frame each D2D paint.

Non-goals:

- The full UI is not yet rendered entirely through this buffer.
- No batching backend yet.
- No GPU instancing or shader rewrite.

Next: route common panel/text draw helpers through command recording in debug mode, then optionally execute from the command buffer.
