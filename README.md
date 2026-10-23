# ManyTerminals 🚀

Ứng dụng Desktop quản lý nhiều màn hình Terminal dạng Lưới (Grid) siêu nhẹ, chuyên dụng để chạy và giám sát nhiều AI Agent hoặc tác vụ CLI đồng thời.

---

## 🌟 Tính năng nổi bật

- **Siêu nhẹ & Tiết kiệm tài nguyên:**
  - Tận dụng `google-chrome-stable --app` có sẵn trong hệ thống (chỉ tốn ~25-35MB RAM, không tốn 150MB+ của Electron).
  - Tích hợp chuẩn xterm.js với Linux PTY (`/bin/bash`), hỗ trợ màu ANSI 256/TrueColor, phím mũi tên, TAB autocomplete, Ctrl+C, vim, htop.
- **Bố cục Lưới động (Dynamic Grid):**
  - Mở mặc định **1 ô** tại thư mục Home (`~`).
  - Tự động co giãn lưới khi thêm/bớt terminal (Auto, 1 Col, 2 Cols, 3 Cols, 4 Cols).
- **Chế độ Zoom (Phóng to / Thu nhỏ):**
  - Bấm phím tắt **`Alt + Z`** hoặc nút góc ô để phóng to 1 terminal chiếm trọn cửa sổ khi cần đọc log dài hoặc nhập prompt cho agent. Bấm lại hoặc phím `Esc` để thu về dạng lưới.
- **Gắn nhãn Agent (Rename/Tag):**
  - Click vào tiêu đề ô để đổi tên (ví dụ: `Agent 1: Coder`, `Agent 2: Scraper`, `Agent 3: Reviewer`).
- **Chế độ Phát đồng loạt (Broadcast Mode):**
  - Bật bằng phím **`Alt + B`** hoặc nút Broadcast trên thanh công cụ.
  - Khi bật, bất kỳ câu lệnh nào bạn gõ sẽ được gửi đồng thời đến **tất cả các terminal**.
- **Tiện ích trên từng ô:**
  - Nút Xóa màn hình (Clear), Khởi động lại Shell (Restart), Đóng ô (Close).

---

## ⌨️ Phím tắt nhanh (Hotkeys)

| Phím tắt | Chức năng |
|---|---|
| **`Alt + N`** | Thêm terminal mới |
| **`Alt + Z`** | Phóng to / Thu nhỏ ô đang chọn (Zoom tile) |
| **`Alt + B`** | Bật / Tắt chế độ Broadcast (gõ gửi tất cả ô) |
| **`Alt + W`** | Đóng ô terminal đang chọn |
| **`Esc`** | Thu nhỏ về lưới nếu đang zoom 1 ô |

---

## 🚀 Cách khởi chạy

### Cách 1: Chạy từ Menu ứng dụng KDE Plasma
Ứng dụng đã được đăng ký vào hệ thống:
1. Mở menu ứng dụng (hoặc bấm `Alt + Space`).
2. Gõ **ManyTerminals** và bấm Enter.

### Cách 2: Chạy bằng dòng lệnh
```bash
cd /home/lighthouse2815/Projects/manyterminals
./start.sh
```

### Chạy kiểm tra hệ thống (Test E2E)
```bash
npm test
```
