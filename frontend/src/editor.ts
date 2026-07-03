import { EditorView, keymap, placeholder } from "@codemirror/view";
import { EditorState, Compartment } from "@codemirror/state";
import { defaultKeymap, history, historyKeymap } from "@codemirror/commands";
import { cpp } from "@codemirror/lang-cpp";
import { oneDark } from "@codemirror/theme-one-dark";

const languageConf = new Compartment();
const tabSizeConf = new Compartment();

export function createEditor(parent: HTMLElement): EditorView {
  const startState = EditorState.create({
    doc: `int printf(char *, ...);\n\nint bump(int *value) {\n    *value = *value + 1;\n    return 0;\n}\n\nint main(void) {\n    int i;\n    int total;\n    int updates;\n    int value;\n    int *p;\n\n    total = 0;\n    for (i = 0; i < 5; i = i + 1) {\n        if (i == 2) {\n            continue;\n        }\n        total = total + i;\n    }\n    printf("%d\\n", total);\n\n    for (i = 0; ; i = i + 1) {\n        if (i == 3) {\n            break;\n        }\n        total = total + 10;\n    }\n    printf("%d\\n", total);\n\n    updates = 0;\n    for (; i < 5; bump(&i)) {\n        updates = updates + 1;\n    }\n    printf("%d\\n", updates);\n    printf("%d\\n", i);\n\n    p = &value;\n    total = 0;\n    for (*p = 0; *p < 3; *p = *p + 1) {\n        total = total + *p;\n    }\n    printf("%d\\n", total);\n\n    total = 0;\n    for (int j = 0, inner = 1; j < 4; j = j + 1) {\n        total = total + j + inner;\n    }\n    printf("%d\\n", total);\n    return 0;\n}\n`,
    extensions: [
      languageConf.of(cpp()),
      tabSizeConf.of(EditorState.tabSize.of(4)),
      history(),
      keymap.of([...defaultKeymap, ...historyKeymap]),
      oneDark,
      placeholder("Write your C code here..."),
      EditorView.lineWrapping,
      EditorView.theme({
        "&": { height: "100%" },
        ".cm-scroller": { overflow: "auto" },
      }),
    ],
  });

  return new EditorView({
    state: startState,
    parent,
  });
}

export function getEditorCode(editor: EditorView): string {
  return editor.state.doc.toString();
}
