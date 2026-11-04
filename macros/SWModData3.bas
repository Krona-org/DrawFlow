Attribute VB_Name = "SWModData3"
Public SWArrayM13(4, 2) As String
Public SWArrayM13Count As Integer
Public SWArrayM13Help(4) As String

Public Sub SWArrayM13Init()
    SWArrayM13(0, 1) = "Сохранение PSW"
    SWArrayM13(1, 1) = "Загрузка по признакам МПС К1804ВС1"
    SWArrayM13(2, 1) = "Загрузка с учетом сдвигов"
    SWArrayM13(3, 1) = "Загрузка с шины Y"
    SWArrayM13Count = 3
    
    SWArrayM13Help(0) = "Разряды PSW - заносимая информация" + vbCrLf _
        + "0-7 - Сохраниение PSW"
    SWArrayM13Help(1) = "Разряды PSW - заносимая информация" + vbCrLf _
        + " 0  - Перенос в старший разряд C16" + vbCrLf _
        + " 1  - Сигнал переполнения OVR" + vbCrLf _
        + " 2  - сигнал нулевого результата Z" + vbCrLf _
        + " 3  - Знак результата F15"
    SWArrayM13Help(2) = "Разряды PSW - заносимая информация" + vbCrLf _
        + " 0  - Выталкиваемый разряд" + vbCrLf _
        + " 1  - Сигнал переполнения OVR" + vbCrLf _
        + " 2  - сигнал нулевого результата Z" + vbCrLf _
        + " 3  - F15 [+] OVR"
    SWArrayM13Help(3) = "Разряды PSW - заносимая информация" + vbCrLf _
        + " 0  - Y[0]" + vbCrLf _
        + " 1  - Y[1]" + vbCrLf _
        + " 2  - Y[2]" + vbCrLf _
        + " 3  - Y[3]" + vbCrLf _
        + " 4  - Y[4]" + vbCrLf _
        + " 5  - Y[5]" + vbCrLf _
        + " 6  - Y[6]" + vbCrLf _
        + " 7  - Y[7]"
End Sub
