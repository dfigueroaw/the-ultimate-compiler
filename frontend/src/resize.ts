export function makeVerticalResize(
  handle: HTMLElement,
  container: HTMLElement,
  paneIndex: number
): void {
  let startY = 0;
  const panes = Array.from(
    container.querySelectorAll(".result-pane")
  ) as HTMLElement[];
  let startHeights: number[] = [];

  handle.addEventListener("mousedown", (e) => {
    e.preventDefault();
    startY = e.clientY;
    startHeights = panes.map((p) => p.getBoundingClientRect().height);
    document.body.style.cursor = "row-resize";
    document.body.style.userSelect = "none";
    document.addEventListener("mousemove", onMove);
    document.addEventListener("mouseup", onUp);
  });

  function onMove(e: MouseEvent) {
    const dy = e.clientY - startY;
    const sizes = [...startHeights];

    const topIdx = paneIndex;
    const botIdx = paneIndex + 1;

    const minH = 60;
    let newTop = sizes[topIdx] + dy;
    let newBot = sizes[botIdx] - dy;

    if (newTop < minH) {
      newBot = newBot - (minH - newTop);
      newTop = minH;
    }
    if (newBot < minH) {
      newTop = newTop - (minH - newBot);
      newBot = minH;
    }

    sizes[topIdx] = Math.max(minH, newTop);
    sizes[botIdx] = Math.max(minH, newBot);

    for (let i = 0; i < panes.length; i++) {
      panes[i].style.flex = `0 0 ${sizes[i]}px`;
    }
  }

  function onUp() {
    document.body.style.cursor = "";
    document.body.style.userSelect = "";
    document.removeEventListener("mousemove", onMove);
    document.removeEventListener("mouseup", onUp);
  }
}

export function makeHorizontalResize(
  handle: HTMLElement,
  left: HTMLElement,
  right: HTMLElement,
  minLeft = 200,
  minRight = 200
): void {
  let startX = 0;
  let startLeft = 0;
  const parent = left.parentElement!;

  handle.addEventListener("mousedown", (e) => {
    e.preventDefault();
    startX = e.clientX;
    startLeft = left.getBoundingClientRect().width;
    document.body.style.cursor = "col-resize";
    document.body.style.userSelect = "none";
    document.addEventListener("mousemove", onMove);
    document.addEventListener("mouseup", onUp);
  });

  function onMove(e: MouseEvent) {
    const dx = e.clientX - startX;
    const parentWidth = parent.getBoundingClientRect().width;
    let newLeft = startLeft + dx;
    newLeft = Math.max(minLeft, Math.min(newLeft, parentWidth - minRight));
    left.style.flex = `0 0 ${newLeft}px`;
    right.style.flex = "1 1 auto";
  }

  function onUp() {
    document.body.style.cursor = "";
    document.body.style.userSelect = "";
    document.removeEventListener("mousemove", onMove);
    document.removeEventListener("mouseup", onUp);
  }
}
