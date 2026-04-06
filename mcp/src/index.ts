#!/usr/bin/env node

import { McpServer } from "@modelcontextprotocol/sdk/server/mcp.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { z } from "zod";
import { Canvas } from "./canvas.js";

const server = new McpServer({
  name: "pspixel",
  version: "1.0.0",
});

// Store active canvases by project name
const projects = new Map<string, Canvas>();

function getProject(name: string): Canvas {
  const canvas = projects.get(name);
  if (!canvas) throw new Error(`No project "${name}". Use create_project first.`);
  return canvas;
}

function parseColor(color: string): [number, number, number, number] {
  // Accept hex (#ff0000, #ff000080) or comma-separated (255,0,0) or (255,0,0,128)
  if (color.startsWith("#")) {
    const hex = color.slice(1);
    const r = parseInt(hex.slice(0, 2), 16);
    const g = parseInt(hex.slice(2, 4), 16);
    const b = parseInt(hex.slice(4, 6), 16);
    const a = hex.length >= 8 ? parseInt(hex.slice(6, 8), 16) : 255;
    return [r, g, b, a];
  }
  const parts = color.split(",").map((s) => parseInt(s.trim()));
  return [parts[0] ?? 0, parts[1] ?? 0, parts[2] ?? 0, parts[3] ?? 255];
}

// --- Tools ---

server.tool(
  "create_project",
  "Create a new pixel art project with the given dimensions",
  { name: z.string(), width: z.number().int().min(1).max(4096), height: z.number().int().min(1).max(4096) },
  async ({ name, width, height }) => {
    const canvas = new Canvas(width, height);
    projects.set(name, canvas);
    return { content: [{ type: "text", text: `Created project "${name}" (${width}x${height}) with 1 layer` }] };
  }
);

server.tool(
  "draw_pixel",
  "Draw a single pixel at (x, y) with the given color",
  { project: z.string(), x: z.number().int(), y: z.number().int(), color: z.string().describe("Hex (#ff0000) or r,g,b or r,g,b,a") },
  async ({ project, x, y, color }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.setPixel(x, y, r, g, b, a);
    return { content: [{ type: "text", text: `Drew pixel at (${x},${y}) color ${color}` }] };
  }
);

server.tool(
  "draw_line",
  "Draw a line from (x0, y0) to (x1, y1)",
  { project: z.string(), x0: z.number().int(), y0: z.number().int(), x1: z.number().int(), y1: z.number().int(), color: z.string() },
  async ({ project, x0, y0, x1, y1, color }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.drawLine(x0, y0, x1, y1, r, g, b, a);
    return { content: [{ type: "text", text: `Drew line (${x0},${y0})→(${x1},${y1})` }] };
  }
);

server.tool(
  "draw_rect",
  "Draw a rectangle",
  { project: z.string(), x: z.number().int(), y: z.number().int(), width: z.number().int(), height: z.number().int(), color: z.string(), filled: z.boolean().default(false) },
  async ({ project, x, y, width, height, color, filled }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.drawRect(x, y, width, height, r, g, b, a, filled);
    return { content: [{ type: "text", text: `Drew ${filled ? "filled " : ""}rect at (${x},${y}) ${width}x${height}` }] };
  }
);

server.tool(
  "draw_circle",
  "Draw a circle at center (cx, cy) with given radius",
  { project: z.string(), cx: z.number().int(), cy: z.number().int(), radius: z.number().int().min(1), color: z.string(), filled: z.boolean().default(false) },
  async ({ project, cx, cy, radius, color, filled }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.drawCircle(cx, cy, radius, r, g, b, a, filled);
    return { content: [{ type: "text", text: `Drew ${filled ? "filled " : ""}circle at (${cx},${cy}) r=${radius}` }] };
  }
);

server.tool(
  "flood_fill",
  "Flood fill from (x, y) with the given color",
  { project: z.string(), x: z.number().int(), y: z.number().int(), color: z.string() },
  async ({ project, x, y, color }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.floodFill(x, y, r, g, b, a);
    return { content: [{ type: "text", text: `Flood filled from (${x},${y}) with ${color}` }] };
  }
);

server.tool(
  "clear_canvas",
  "Clear the active layer (or fill with a color)",
  { project: z.string(), color: z.string().default("#00000000").describe("Default transparent") },
  async ({ project, color }) => {
    const canvas = getProject(project);
    const [r, g, b, a] = parseColor(color);
    canvas.clear(r, g, b, a);
    return { content: [{ type: "text", text: `Cleared active layer` }] };
  }
);

server.tool(
  "add_layer",
  "Add a new layer to the project",
  { project: z.string(), name: z.string() },
  async ({ project, name }) => {
    const canvas = getProject(project);
    const idx = canvas.addLayer(name);
    canvas.setActiveLayer(idx);
    return { content: [{ type: "text", text: `Added layer "${name}" (index ${idx}), now active` }] };
  }
);

server.tool(
  "set_active_layer",
  "Switch the active layer by index",
  { project: z.string(), index: z.number().int() },
  async ({ project, index }) => {
    const canvas = getProject(project);
    if (!canvas.setActiveLayer(index)) throw new Error(`Invalid layer index ${index}`);
    return { content: [{ type: "text", text: `Active layer: ${canvas.activeLayer.name} (index ${index})` }] };
  }
);

server.tool(
  "list_layers",
  "List all layers in the project",
  { project: z.string() },
  async ({ project }) => {
    const canvas = getProject(project);
    const lines = canvas.layers.map((l, i) =>
      `${i === canvas.activeLayerIndex ? "→ " : "  "}[${i}] ${l.name} ${l.visible ? "👁" : "hidden"} ${Math.round(l.opacity * 100)}%`
    );
    return { content: [{ type: "text", text: lines.join("\n") }] };
  }
);

server.tool(
  "set_layer_visibility",
  "Show or hide a layer",
  { project: z.string(), index: z.number().int(), visible: z.boolean() },
  async ({ project, index, visible }) => {
    const canvas = getProject(project);
    if (index < 0 || index >= canvas.layers.length) throw new Error("Invalid layer index");
    canvas.layers[index].visible = visible;
    return { content: [{ type: "text", text: `Layer ${index} "${canvas.layers[index].name}" ${visible ? "shown" : "hidden"}` }] };
  }
);

server.tool(
  "set_layer_opacity",
  "Set a layer's opacity (0.0 to 1.0)",
  { project: z.string(), index: z.number().int(), opacity: z.number().min(0).max(1) },
  async ({ project, index, opacity }) => {
    const canvas = getProject(project);
    if (index < 0 || index >= canvas.layers.length) throw new Error("Invalid layer index");
    canvas.layers[index].opacity = opacity;
    return { content: [{ type: "text", text: `Layer ${index} opacity set to ${Math.round(opacity * 100)}%` }] };
  }
);

server.tool(
  "export_png",
  "Export the composited canvas as a PNG file",
  { project: z.string(), path: z.string() },
  async ({ project, path: filePath }) => {
    const canvas = getProject(project);
    const absPath = await canvas.exportPng(filePath);
    return { content: [{ type: "text", text: `Exported to ${absPath}` }] };
  }
);

server.tool(
  "export_all_layers",
  "Export each layer as a separate PNG to a directory",
  { project: z.string(), directory: z.string() },
  async ({ project, directory }) => {
    const canvas = getProject(project);
    const paths = await canvas.exportAllLayers(directory);
    return { content: [{ type: "text", text: `Exported ${paths.length} layers:\n${paths.join("\n")}` }] };
  }
);

server.tool(
  "get_canvas_info",
  "Get project info: dimensions, layer count, active layer",
  { project: z.string() },
  async ({ project }) => {
    const canvas = getProject(project);
    const info = {
      width: canvas.width,
      height: canvas.height,
      layers: canvas.layers.length,
      activeLayer: canvas.activeLayerIndex,
      activeLayerName: canvas.activeLayer.name,
    };
    return { content: [{ type: "text", text: JSON.stringify(info, null, 2) }] };
  }
);

server.tool(
  "list_projects",
  "List all open projects",
  {},
  async () => {
    if (projects.size === 0) return { content: [{ type: "text", text: "No projects open" }] };
    const lines = Array.from(projects.entries()).map(([name, c]) => `${name}: ${c.width}x${c.height}, ${c.layers.length} layers`);
    return { content: [{ type: "text", text: lines.join("\n") }] };
  }
);

// --- Start server ---

async function main() {
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((err) => {
  console.error("Fatal:", err);
  process.exit(1);
});
