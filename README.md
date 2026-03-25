# graph-sandbox
A C++ program simulates graph traversal using DFS and BFS algorithms to find intersecting vertices and connecting edges of the graph.

# Graph Sandbox 🔬
### Công cụ trực quan hoá đồ thị — Bridge & Articulation Point

---

## Cấu trúc file
```
ng2/
├── main.cpp       ← Entry point
├── app.h          ← Toàn bộ logic + render + events
├── graph.h        ← Đồ thị + Tarjan + sinh bước DFS/BFS
├── animator.h     ← Step animator có speed control
├── renderer.h     ← SDL2 primitives, Button, Slider
├── info_panel.h   ← Panel toggle: adj list / matrix / disc-low
├── build.sh       ← MSYS2 build script
└── assets/
    └── font.ttf   ← Copy vào trước khi build
```

---

## Cách dùng

### Tạo đồ thị
| Nút / Phím | Tác dụng |
|---|---|
| **Add Node** / N | Click vào canvas để tạo đỉnh |
| **Add Edge** / E | Drag từ đỉnh A sang đỉnh B để nối cạnh |
| **Delete** / D | Click đỉnh hoặc cạnh để xóa |
| **Reset Graph** / R | Xóa toàn bộ đồ thị |

### Phân tích
| Nút / Phím | Tác dụng |
|---|---|
| **Run Tarjan** / ENTER | Chạy Tarjan → tô màu bridge 🔴 và art.point 🟠 |
| **Animate DFS** | Chạy DFS animation từng bước từ node 0 |
| **Animate BFS** | Chạy BFS animation từng bước từ node 0 |
| **Skip Anim** / SPACE | Bỏ qua animation ngay lập tức |
| **Clear Result** | Xóa kết quả phân tích |
| Speed slider | Điều chỉnh tốc độ animation (0.3x → 12x) |

### Info Panel
| Nút / Phím | Tác dụng |
|---|---|
| **Info Panel** / I | Toggle panel bên phải |
| Tab **Adj.List** | Danh sách kề của mỗi đỉnh |
| Tab **Matrix** | Ma trận kề (1/·), bridge tô đỏ |
| Tab **disc/low** | Bảng disc[], low[], loại Art/Bridge |

---

## Màu sắc
| Màu | Ý nghĩa |
|---|---|
| 🔵 Xanh lam | Node bình thường |
| 🟢 Xanh lá | Node đã được DFS/BFS thăm |
| 🟠 Cam (glow) | **Articulation Point** |
| 🔴 Đỏ (cạnh) | **Bridge** |
| 🔵 Xanh nhạt (cạnh) | DFS tree edge |
