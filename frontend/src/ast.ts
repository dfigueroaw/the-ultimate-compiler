import * as d3 from "d3";

interface AstNode {
  name: string;
  type: string;
  label: string;
  children: AstNode[];
}

let currentZoom: d3.ZoomBehavior<SVGSVGElement, unknown> | null = null;
let currentSvg: d3.Selection<SVGSVGElement, unknown, null, undefined> | null = null;
let currentG: d3.Selection<SVGGElement, unknown, null, undefined> | null = null;

export function parseAst(text: string): AstNode {
  const root: AstNode = { name: "(empty)", type: "", label: "", children: [] };
  const stack: { node: AstNode; depth: number }[] = [{ node: root, depth: -1 }];

  for (const line of text.split("\n")) {
    const trimmed = line.trim();
    if (!trimmed) continue;

    const leading = line.length - line.trimStart().length;
    const depth = leading / 2;

    const colonIdx = trimmed.indexOf(":");
    let type: string;
    let label: string;
    if (colonIdx === -1) {
      type = trimmed;
      label = "";
    } else {
      type = trimmed.slice(0, colonIdx);
      label = trimmed.slice(colonIdx + 1).trim();
    }

    const node: AstNode = { name: trimmed, type, label, children: [] };

    while (stack.length > 1 && stack[stack.length - 1].depth >= depth) {
      stack.pop();
    }
    stack[stack.length - 1].node.children.push(node);
    stack.push({ node, depth });
  }

  return root.children[0] || root;
}

export function renderAstTree(
  container: HTMLElement,
  root: AstNode
): void {
  container.innerHTML = "";

  const rect = container.getBoundingClientRect();
  const width = Math.max(rect.width, 400);
  const height = Math.max(rect.height, 300);

  currentSvg = d3
    .select(container)
    .append("svg")
    .attr("width", width)
    .attr("height", height)
    .style("background", "transparent")
    .style("cursor", "grab");

  const treemap = d3.tree<AstNode>().nodeSize([180, 64]);
  const hierarchy = d3.hierarchy(root);
  const treeData = treemap(hierarchy);

  const nodes = treeData.descendants();
  const links = treeData.links();

  const maxX = d3.max(nodes, (d) => d.x) || 0;
  const maxY = d3.max(nodes, (d) => d.y) || 0;

  const svgW = Math.max(width, maxX + 120);
  const svgH = Math.max(height, maxY + 60);
  currentSvg.attr("width", svgW).attr("height", svgH);

  currentG = currentSvg.append("g");

  currentG
    .selectAll("path.link")
    .data(links)
    .join("path")
    .attr("class", "link")
    .attr(
      "d",
      (d) =>
        `M${d.source.x},${d.source.y} C${d.source.x},${(d.source.y + d.target.y) / 2} ${d.target.x},${(d.source.y + d.target.y) / 2} ${d.target.x},${d.target.y}`
    )
    .attr("fill", "none")
    .attr("stroke", "#363854")
    .attr("stroke-width", 1.5);

  const colorScale = d3.scaleOrdinal(d3.schemeCategory10);

  const nodeGroup = currentG
    .selectAll("g.node")
    .data(nodes)
    .join("g")
    .attr("class", "node")
    .attr("transform", (d) => `translate(${d.x},${d.y})`);

  nodeGroup
    .append("circle")
    .attr("r", 10)
    .attr("fill", (d) => colorScale(d.data.type || d.data.name))
    .attr("stroke", "#1a1b26")
    .attr("stroke-width", 2)
    .style("cursor", "pointer")
    .on("mouseenter", function () {
      d3.select(this).attr("r", 14);
    })
    .on("mouseleave", function () {
      d3.select(this).attr("r", 10);
    });

  nodeGroup
    .append("text")
    .attr("dy", -14)
    .attr("text-anchor", "middle")
    .text((d) =>
      d.data.label ? `${d.data.type}: ${d.data.label}` : d.data.type
    )
    .attr("fill", "#c9d1d9")
    .attr("font-family", '"JetBrains Mono", monospace')
    .attr("font-size", "11px")
    .style("pointer-events", "none");

  currentZoom = d3.zoom<SVGSVGElement, unknown>()
    .scaleExtent([0.1, 10])
    .on("zoom", (event) => {
      if (currentG) {
        currentG.attr("transform", event.transform);
      }
    });

  currentSvg.call(currentZoom);
  currentSvg.on("dblclick.zoom", null);

  currentSvg
    .on("mousedown", () => currentSvg?.style("cursor", "grabbing"))
    .on("mouseup", () => currentSvg?.style("cursor", "grab"));
}

export function zoomAstIn(): void {
  if (currentSvg && currentZoom) {
    currentSvg.transition().duration(200).call(currentZoom.scaleBy, 1.3);
  }
}

export function zoomAstOut(): void {
  if (currentSvg && currentZoom) {
    currentSvg.transition().duration(200).call(currentZoom.scaleBy, 0.7);
  }
}

export function resetAstZoom(): void {
  if (currentSvg && currentZoom) {
    currentSvg.transition().duration(300).call(currentZoom.transform, d3.zoomIdentity);
  }
}

export function renderAstInModal(
  container: HTMLElement,
  astText: string
): void {
  const parsed = parseAst(astText);
  renderAstTree(container, parsed);
}
