![69-imgui-loader-ezgif com-optimize](https://github.com/user-attachments/assets/6dea1fba-870c-4a35-8c0d-cea43f0ab836)

### วิธี Build
1. เปิด Project ด้วย Visual Studio 2022
2. เลือก Release | x64
3. Build

### แก้ไขระบบ License, Software
1. ไปที่ไฟล์ `69-ImGui-Loader\src\service\KeyauthService.cpp`
2. แก้ไขระบบ License โดยดูจากตัวอย่างที่ทำไว้
3. แก้ไขชื่อ Software และวิธีการ Launch โดยดูจากตัวอย่างที่ทำไว้

### เปลี่ยนรูป Logo และรูป Software
1. ไปที่โฟลเดอร์ `69-ImGui-Loader\scripts`
2. แก้ไขรูป logo.png ตามต้องการ
3. แก้ไขรูป software.png ตามต้องการ
4. รัน script `py change_logo.py` เพื่อเปลี่ยนรูป Logo ลงใน Project อัตโนมัติ 
5. รัน script `py change_software.py` เพื่อเปลี่ยนรูป Software ลงใน Project อัตโนมัติ
6. Build โปรแกรมใหม่

### เปลี่ยนสี Menu
1. ไปที่ไฟล์ `69-ImGui-Loader\include\69\menu\Theme.h`
2. แก้ไขค่าสี `const ImVec4` ต่างๆได้เลย
