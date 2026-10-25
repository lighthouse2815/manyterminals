# ManyTerminals ⚡

Ứng dụng Desktop quản lý nhiều màn hình Terminal dạng Lưới (Grid) **Native Qt6** siêu nhẹ, chuyên dụng để chạy và giám sát nhiều AI Agent hoặc tác vụ CLI đồng thời.

---

## 🌟 Tính năng nổi bật

- **Thanh công cụ gọn như Chrome:** Không còn hàng tiêu đề riêng phía trên; nút thu nhỏ, phóng to/khôi phục và đóng nằm ở góc phải thanh công cụ. Kéo vùng trống trên thanh để di chuyển, nhấp đúp để phóng to/khôi phục; kéo góc dưới phải để đổi kích thước cửa sổ nổi.
- **Native Qt6 Thuần túy (Siêu nhẹ & Tiết kiệm tối đa):**
  - Tích hợp trực tiếp engine terminal **KDE KonsolePart** gốc của hệ điều hành.
  - Dung lượng file nhị phân chỉ **~108 KB**.
  - Bộ nhớ RAM tiêu thụ chỉ khoảng **~30MB** (nhẹ hơn gấp 5 lần so với Chrome/Electron).
  - Khởi động tức thì trong chớp mắt (~0.05 giây).
  - Hỗ trợ đầy đủ phím mũi tên, TAB autocomplete, màu sắc ANSI TrueColor, phím tắt `Ctrl+C`, `vim`, `htop`, font rendering chống răng cưa mượt mà.
- **Bố cục Lưới động (Dynamic Grid):**
  - Mở mặc định **1 ô** tại thư mục Home (`~`).
  - Tự động chia đều lưới khi mở thêm ô (Auto, 1 Cột, 2 Cột, 3 Cột, 4 Cột).
- **Chế độ Zoom (Phóng to / Thu nhỏ):**
  - Nhấn phím **`Alt + Z`** hoặc nút phóng to góc ô để bung to 1 ô chiếm trọn màn hình khi cần tập trung đọc log hoặc nhập prompt cho agent. Nhấn lại hoặc phím `Esc` để thu về lưới.
- **Gắn nhãn Agent (Rename):**
  - Click vào tiêu đề ô để đổi tên (ví dụ: `Agent Coder`, `Agent Scraper`, `Agent Reviewer`).
- **Chế độ Broadcast (Phát lệnh đồng loạt):**
  - Bật bằng phím **`Alt + B`** hoặc nút Broadcast trên thanh công cụ.
  - Khi bật, mọi ký tự và câu lệnh bạn gõ sẽ được phát đồng thời đến **tất cả các terminal**.
- **Tiện ích trên từng ô:**
  - Nút Xóa màn hình (Clear), Phóng to (Zoom), Đóng ô (Close).

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

### Cách 1: Mở từ Menu ứng dụng KDE Plasma (Khuyên dùng)
Ứng dụng đã được tích hợp sẵn vào hệ thống:
1. Bấm phím `Super` (Windows) hoặc `Alt + Space`.
2. Gõ **ManyTerminals** và nhấn Enter.

### Cách 2: Chạy bằng dòng lệnh
```bash
cd /home/lighthouse2815/Projects/manyterminals
./start.sh
```

### Biên dịch thủ công (Build with CMake)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/manyterminals
```
