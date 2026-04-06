import sharp from "sharp";
import * as fs from "fs/promises";
import * as path from "path";

export interface Layer {
  name: string;
  pixels: Uint8ClampedArray; // RGBA flat array
  visible: boolean;
  opacity: number;
}

export class Canvas {
  width: number;
  height: number;
  layers: Layer[];
  activeLayerIndex: number;

  constructor(width: number, height: number) {
    this.width = width;
    this.height = height;
    this.layers = [];
    this.activeLayerIndex = 0;
    this.addLayer("Background");
  }

  addLayer(name: string): number {
    const pixels = new Uint8ClampedArray(this.width * this.height * 4);
    this.layers.push({ name, pixels, visible: true, opacity: 1.0 });
    return this.layers.length - 1;
  }

  removeLayer(index: number): boolean {
    if (this.layers.length <= 1 || index < 0 || index >= this.layers.length) return false;
    this.layers.splice(index, 1);
    if (this.activeLayerIndex >= this.layers.length) {
      this.activeLayerIndex = this.layers.length - 1;
    }
    return true;
  }

  setActiveLayer(index: number): boolean {
    if (index < 0 || index >= this.layers.length) return false;
    this.activeLayerIndex = index;
    return true;
  }

  get activeLayer(): Layer {
    return this.layers[this.activeLayerIndex];
  }

  setPixel(x: number, y: number, r: number, g: number, b: number, a: number = 255): void {
    if (x < 0 || x >= this.width || y < 0 || y >= this.height) return;
    const layer = this.activeLayer;
    const idx = (y * this.width + x) * 4;
    layer.pixels[idx] = r;
    layer.pixels[idx + 1] = g;
    layer.pixels[idx + 2] = b;
    layer.pixels[idx + 3] = a;
  }

  getPixel(x: number, y: number): [number, number, number, number] {
    if (x < 0 || x >= this.width || y < 0 || y >= this.height) return [0, 0, 0, 0];
    const layer = this.activeLayer;
    const idx = (y * this.width + x) * 4;
    return [layer.pixels[idx], layer.pixels[idx + 1], layer.pixels[idx + 2], layer.pixels[idx + 3]];
  }

  fillRect(x: number, y: number, w: number, h: number, r: number, g: number, b: number, a: number = 255): void {
    for (let py = y; py < y + h; py++) {
      for (let px = x; px < x + w; px++) {
        this.setPixel(px, py, r, g, b, a);
      }
    }
  }

  drawLine(x0: number, y0: number, x1: number, y1: number, r: number, g: number, b: number, a: number = 255): void {
    // Bresenham's line algorithm
    const dx = Math.abs(x1 - x0);
    const dy = Math.abs(y1 - y0);
    const sx = x0 < x1 ? 1 : -1;
    const sy = y0 < y1 ? 1 : -1;
    let err = dx - dy;
    let x = x0, y = y0;

    while (true) {
      this.setPixel(x, y, r, g, b, a);
      if (x === x1 && y === y1) break;
      const e2 = 2 * err;
      if (e2 > -dy) { err -= dy; x += sx; }
      if (e2 < dx) { err += dx; y += sy; }
    }
  }

  drawCircle(cx: number, cy: number, radius: number, r: number, g: number, b: number, a: number = 255, filled: boolean = false): void {
    if (filled) {
      for (let py = cy - radius; py <= cy + radius; py++) {
        for (let px = cx - radius; px <= cx + radius; px++) {
          const dx = px - cx, dy = py - cy;
          if (dx * dx + dy * dy <= radius * radius) {
            this.setPixel(px, py, r, g, b, a);
          }
        }
      }
    } else {
      // Midpoint circle
      let x = 0, y = radius, d = 3 - 2 * radius;
      const plot = (cx: number, cy: number, x: number, y: number) => {
        for (const [px, py] of [[cx+x,cy+y],[cx-x,cy+y],[cx+x,cy-y],[cx-x,cy-y],[cx+y,cy+x],[cx-y,cy+x],[cx+y,cy-x],[cx-y,cy-x]]) {
          this.setPixel(px, py, r, g, b, a);
        }
      };
      plot(cx, cy, x, y);
      while (y >= x) {
        x++;
        d = d > 0 ? (y--, d + 4*(x-y) + 10) : d + 4*x + 6;
        plot(cx, cy, x, y);
      }
    }
  }

  drawRect(x: number, y: number, w: number, h: number, r: number, g: number, b: number, a: number = 255, filled: boolean = false): void {
    if (filled) {
      this.fillRect(x, y, w, h, r, g, b, a);
    } else {
      for (let px = x; px < x + w; px++) { this.setPixel(px, y, r, g, b, a); this.setPixel(px, y + h - 1, r, g, b, a); }
      for (let py = y; py < y + h; py++) { this.setPixel(x, py, r, g, b, a); this.setPixel(x + w - 1, py, r, g, b, a); }
    }
  }

  floodFill(x: number, y: number, r: number, g: number, b: number, a: number = 255): void {
    if (x < 0 || x >= this.width || y < 0 || y >= this.height) return;
    const target = this.getPixel(x, y);
    const fill = [r, g, b, a] as [number, number, number, number];
    if (target[0] === fill[0] && target[1] === fill[1] && target[2] === fill[2] && target[3] === fill[3]) return;

    const stack: [number, number][] = [[x, y]];
    while (stack.length > 0) {
      const [px, py] = stack.pop()!;
      if (px < 0 || px >= this.width || py < 0 || py >= this.height) continue;
      const cur = this.getPixel(px, py);
      if (cur[0] !== target[0] || cur[1] !== target[1] || cur[2] !== target[2] || cur[3] !== target[3]) continue;
      this.setPixel(px, py, r, g, b, a);
      stack.push([px+1, py], [px-1, py], [px, py+1], [px, py-1]);
    }
  }

  clear(r: number = 0, g: number = 0, b: number = 0, a: number = 0): void {
    const layer = this.activeLayer;
    for (let i = 0; i < layer.pixels.length; i += 4) {
      layer.pixels[i] = r;
      layer.pixels[i+1] = g;
      layer.pixels[i+2] = b;
      layer.pixels[i+3] = a;
    }
  }

  composite(): Uint8ClampedArray {
    const result = new Uint8ClampedArray(this.width * this.height * 4);
    for (const layer of this.layers) {
      if (!layer.visible) continue;
      for (let i = 0; i < result.length; i += 4) {
        const sa = (layer.pixels[i+3] / 255) * layer.opacity;
        const da = result[i+3] / 255;
        const oa = sa + da * (1 - sa);
        if (oa === 0) continue;
        result[i]   = (layer.pixels[i]   * sa + result[i]   * da * (1 - sa)) / oa;
        result[i+1] = (layer.pixels[i+1] * sa + result[i+1] * da * (1 - sa)) / oa;
        result[i+2] = (layer.pixels[i+2] * sa + result[i+2] * da * (1 - sa)) / oa;
        result[i+3] = oa * 255;
      }
    }
    return result;
  }

  async exportPng(filePath: string): Promise<string> {
    const pixels = this.composite();
    const buffer = Buffer.from(pixels.buffer);
    await sharp(buffer, { raw: { width: this.width, height: this.height, channels: 4 } })
      .png()
      .toFile(filePath);
    return path.resolve(filePath);
  }

  async exportLayerPng(layerIndex: number, filePath: string): Promise<string> {
    if (layerIndex < 0 || layerIndex >= this.layers.length) throw new Error("Invalid layer index");
    const layer = this.layers[layerIndex];
    const buffer = Buffer.from(layer.pixels.buffer);
    await sharp(buffer, { raw: { width: this.width, height: this.height, channels: 4 } })
      .png()
      .toFile(filePath);
    return path.resolve(filePath);
  }

  async exportAllLayers(dirPath: string): Promise<string[]> {
    await fs.mkdir(dirPath, { recursive: true });
    const paths: string[] = [];
    for (let i = 0; i < this.layers.length; i++) {
      const safeName = this.layers[i].name.replace(/[^a-zA-Z0-9_\- ]/g, "_");
      const filePath = path.join(dirPath, `${String(i).padStart(2, "0")}_${safeName}.png`);
      await this.exportLayerPng(i, filePath);
      paths.push(filePath);
    }
    return paths;
  }
}
