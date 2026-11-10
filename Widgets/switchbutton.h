#ifndef SWITCHBUTTON_H
#define SWITCHBUTTON_H

#include <VisorGUI/Widgets/abstractwidget.h>


namespace vgui {


// Кнопка с переключаемым состоянием
class SwitchButton : public AbstractWidget
{
public:
    SwitchButton(MySize<double> standardWidgetSize,
                 glm::dvec2 standardWidgetPos,
                 SimplePair<SIZE_POLICY> sizePolicy = {FIXED_SIZE, FIXED_SIZE},
                 SimplePair<POSITIONING_POLICY> positioningPolicy = {ABSOLUTE_WINDOW_POSITION, ABSOLUTE_WINDOW_POSITION},
                 glm::dvec2 nullOffset = glm::dvec2(0, 0),
                 std::wstring text = std::wstring(),
                 bool isLightType = false);
    
    // Сигналы
    class StateChange_Event : public Event {
    public:
        bool state;
        StateChange_Event(void* sender, bool state) : Event(sender), state(state) {}
    };
    
    // Изменение состояния
    void setState(bool newState, bool emitSignal = true);
    
    // Обработка пользовательского ввода
    bool handleEvent(const SDL_Event& event) override;
    
    // Курсор покинул родительский виджет
    void cursorLeftParentWidget() override;
    
    // Добавляет связанные кнопки. Когда кто-то получит состояние true, все остальные кнопки в связки получат false
    void addRelatedButtons(std::vector<SwitchButton*> &buttons);
    void addRelatedButtons(SwitchButton**, int size);
    // Статические методы, которые сами вызовут addRelatedButtons для каждой кнопки
    static void linkButtons(std::initializer_list<SwitchButton*> buttons);
    static void linkButtons(std::vector<SwitchButton*> &buttons);
    static void linkButtons(SwitchButton**, int size);

    // Текущее состояние
    bool getState() {return state;}

    // Задать цвет текста для обоих состояний
    void setTextColor(glm::ivec3 textColorActivated, glm::ivec3 textColorDeactivated) {
        if (this->textColorActivated != textColorActivated || this->textColorDeactivated != textColorDeactivated) {
            this->textColorActivated = textColorActivated;
            this->textColorDeactivated = textColorDeactivated;
            updateLater();
        }
    }

    // Задать цвет фона включённой кнопки (выключенная темнее)
    void setBackgroundColor(glm::ivec3 backgroundColor) {
        if (backgroundBaseColor != backgroundColor) {
            backgroundBaseColor = backgroundColor;
            updateLater();
        }
    }
    
    // Включение запрета на отключение через ui. Для групп, где всегда должен быть выбран один из вариантов
    void setNotDeactivateMode() {isNotDeactivate = true;}

    // Заблокировать/разблокировать кнопку

protected:
    
    // Создание текстуры
    virtual void updateTexture() override;

    
    bool isCursorOnButton = false; // Курсор над кнопкой
    bool isButtonPress = false; // Курсор зажат
    
    bool state = false; // Текущее состояние

    bool isLightType = false; // Если облегчённая версия, то не рисуются границы и фон прозрачный
    
    // Запрет на отключение через ui. Для групп, где всегда должен быть выбран один из вариантов
    bool isNotDeactivate = false;

    // Слот изменения состояния связанных кнопок
    void relatedButtonStateChange_Slot(const SwitchButton::StateChange_Event& event);


    // Цвет текста
    glm::ivec3 textColorActivated = {255, 255, 255};
    glm::ivec3 textColorDeactivated = {200, 200, 200};

    // Цвет фона включённой кнопки, выключенная - в 0.6 от него
    glm::ivec3 backgroundBaseColor = {100, 100, 100};
    
};




} // namespace vgui

#endif // SWITCHBUTTON_H
