Attribute VB_Name = "SWModData5"
Public SWArrayM4(16, 2) As String
Public SWArrayM4Short(16) As String
Public SWArrayM4Count As Integer

Public SWArrayM2(4, 2) As String
Public SWArrayM2Count As Integer

Public SWArrayM3(16, 2) As String
Public SWArrayM3Count As Integer

Public Sub SWArrayM4Init()
    SWArrayM4(0, 1) = "Перейти на нулевой адрес (JZ)"
    SWArrayM4(1, 1) = "Условный переход к подпрограмме по адресу из RMK (CJS)"
    SWArrayM4(2, 1) = "Переход по адресу из старшего байта RK (JMAP)"
    SWArrayM4(3, 1) = "Условный переход по адресу из RMK (CJP)"
    SWArrayM4(4, 1) = "Загрузка стека и условная загрузка счетчика СТ (PUSH)"
    SWArrayM4(5, 1) = "Условный переход к подпрограмме по адресу из RA или RMK (JSRP)"
    SWArrayM4(6, 1) = "Условный переход по адресу из младшего байта RK (CJV)"
    SWArrayM4(7, 1) = "Условный переход на адрес из RA или из RMK (JRP)"
    SWArrayM4(8, 1) = "Повторение цикла, если счетчик не равен нулю (RFCT)"
    SWArrayM4(9, 1) = "Повторение по адресу из RMK, если счетчик не равен нулю (RPCT)"
    SWArrayM4(10, 1) = "Условный возврат из подпрограммы (CRTN)"
    SWArrayM4(11, 1) = "Условный переход по адресу из RMK и прием из стека (CJPP)"
    SWArrayM4(12, 1) = "Загрузка счетчика и последовательная выборка (LDCT)"
    SWArrayM4(13, 1) = "Проверка условия окончания цикла (LOOP)"
    SWArrayM4(14, 1) = "Последовательная выборка (CONT)"
    SWArrayM4(15, 1) = "Переход по адресу, выбираемому из RMK (JP)"
    
    SWArrayM4Short(0) = "JZ"
    SWArrayM4Short(1) = "CJS"
    SWArrayM4Short(2) = "JMAP"
    SWArrayM4Short(3) = "CJP"
    SWArrayM4Short(4) = "PUSH"
    SWArrayM4Short(5) = "JSRP"
    SWArrayM4Short(6) = "CJV"
    SWArrayM4Short(7) = "JRP"
    SWArrayM4Short(8) = "RFCT"
    SWArrayM4Short(9) = "RPCT"
    SWArrayM4Short(10) = "CRTN"
    SWArrayM4Short(11) = "CJPP"
    SWArrayM4Short(12) = "LDCT"
    SWArrayM4Short(13) = "LOOP"
    SWArrayM4Short(14) = "CONT"
    SWArrayM4Short(15) = "JP"
    
    SWArrayM4Count = 15
End Sub

Public Sub SWArrayM2Init()
    SWArrayM2(0, 1) = "Нет инверсии условия"
    SWArrayM2(1, 1) = "Инверсия условия"
    SWArrayM2(2, 1) = "Нет инверсии условия (маска)"
    SWArrayM2(3, 1) = "Инверсия условия (маска)"
    SWArrayM2Count = 3
End Sub

Public Sub SWArrayM3Init()
    SWArrayM3(0, 1) = "PSW[0]"
    SWArrayM3(1, 1) = "PSW[1]"
    SWArrayM3(2, 1) = "PSW[2]"
    SWArrayM3(3, 1) = "PSW[3]"
    SWArrayM3(4, 1) = "PSW[4]"
    SWArrayM3(5, 1) = "PSW[5]"
    SWArrayM3(6, 1) = "Перенос СТ"
    SWArrayM3(7, 1) = "Константа 0"
    SWArrayM3(8, 1) = "FLG[0]"
    SWArrayM3(9, 1) = "FLG[1]"
    SWArrayM3(10, 1) = "FLG[2]"
    SWArrayM3(11, 1) = "FLG[3]"
    SWArrayM3(12, 1) = "FLG[4]"
    SWArrayM3(13, 1) = "FLG[5]"
    SWArrayM3(14, 1) = "FLG[6]"
    SWArrayM3(15, 1) = "FLG[7]"
    SWArrayM3Count = 15
End Sub

Public Sub SWArrayM3InitAlt()
    SWArrayM3(0, 1) = "PSW[0]"
    SWArrayM3(1, 1) = "PSW[1]"
    SWArrayM3(2, 1) = "PSW[2]"
    SWArrayM3(3, 1) = "PSW[3]"
    SWArrayM3(4, 1) = "PSW[4]"
    SWArrayM3(5, 1) = "PSW[5]"
    SWArrayM3(6, 1) = "Перенос СТ"
    SWArrayM3(7, 1) = "Константа 0"
    SWArrayM3(8, 1) = "УР1"
    SWArrayM3(9, 1) = "УР2.1"
    SWArrayM3(10, 1) = "УР2.2"
    SWArrayM3(11, 1) = "УР3"
    SWArrayM3(12, 1) = "УР4"
    SWArrayM3(13, 1) = "УР5"
    SWArrayM3(14, 1) = "RK[11/9]=0"
    SWArrayM3(15, 1) = "RK[5/3]=0"
    SWArrayM3Count = 15
End Sub
