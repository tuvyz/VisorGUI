#include <VisorGUI/Widgets/slider.h>


namespace vgui {

Slider::Slider(MySize<double> standardWidgetSize,
               glm::dvec2 standardWidgetPos,
               SimplePair<SIZE_POLICY> sizePolicy,
               SimplePair<POSITIONING_POLICY> positioningPolicy,
               glm::dvec2 nullOffset)
{
    cursorType = SDL_SYSTEM_CURSOR_HAND;

    initWidget(standardWidgetSize, standardWidgetPos, sizePolicy, positioningPolicy, nullOffset);
}



// Обработка пользовательского ввода
bool Slider::handleEvent(const SDL_Event& event) {

    if (!isVisible)
        return false;

    // Проверка на нахождение курсора в виджете (ползунок или стрелки)
    bool isCursorInWidget = cursorInWidget();
    if (!isSliderPress && pressedArrow == 0 && !isCursorInWidget) {

        if (isCursorOnSlider || hoveredArrow != 0) {
            isCursorOnSlider = false;
            hoveredArrow = 0;
            updateTexture();
        }

        return false;
    }


    // Если заблокировано, то выходим
    if (isBlocked)
        return false;


    switch (event.type) {

    case SDL_MOUSEMOTION: { // Перемещение ползунка

        if (isSliderPress) {
            setSliderAbsPos(guiContext->cursorPosition().x - widgetPos.x);
            updateTexture();
            return true;
        }

        break;

    }

    case SDL_MOUSEBUTTONDOWN: {

        // Зажатие стрелки: первый шаг на ближайшем рендере, дальше автоповтор там же, пока стрелка зажата
        int arrow = arrowUnderCursor();
        if (arrow != 0) {
            pressedArrow = arrow;
            arrowHoldTimer.start();
            arrowHoldLastTime = 0;
            arrowStepsAccumulated = 1;
        } else {
            // Зажатие ползунка
            isSliderPress = true;
        }
        setFocus();
        updateTexture();

        return true;
    }
    case SDL_MOUSEBUTTONUP: {

        // Отпуск ползунка или стрелки
        if (isSliderPress || pressedArrow != 0) {
            isSliderPress = false;
            pressedArrow = 0;
            resetFocus();
            updateTexture();
            return true;
        }

        break;
    }
    }


    // Подсветка того, что под курсором: ползунка или стрелки (они не пересекаются)
    int arrow = isCursorInWidget ? arrowUnderCursor() : 0;
    bool isCursorOnSliderNow = isCursorInWidget && arrow == 0;
    if (isCursorOnSlider != isCursorOnSliderNow || hoveredArrow != arrow) {
        isCursorOnSlider = isCursorOnSliderNow;
        hoveredArrow = arrow;
        updateTexture();
    }

    return false;
}





// Рендер: пока зажата стрелка, двигает значение автоповтором
void Slider::render() {

    if (isVisible && pressedArrow != 0 && !isBlocked) {
        double repeatDelay = 0.4; // Задержка автоповтора после нажатия, с
        double repeatStartRate = 10; // Скорость автоповтора в начале, шагов в секунду; удваивается каждую секунду удержания
        double repeatMaxRate = 200; // Предельная скорость автоповтора, шагов в секунду

        // Шаги автоповтора копятся, пока курсор над зажатой стрелкой (увёл курсор - пауза, как у полос прокрутки)
        double holdTime = arrowHoldTimer.s();
        if (holdTime > repeatDelay && arrowUnderCursor() == pressedArrow) {
            double repeatRate = (std::min)(repeatStartRate * std::exp2(holdTime - repeatDelay), repeatMaxRate);
            arrowStepsAccumulated += repeatRate * (holdTime - (std::max)(arrowHoldLastTime, repeatDelay));
        }
        arrowHoldLastTime = holdTime;

        // Шаг - деление слайдера, у плавного - сотая часть диапазона; знак - в сторону правого края.
        // Значение ходит по сетке шагов: у плавного после перетаскивания (0.347) первый шаг - до ближайшего деления в сторону стрелки (0.35 или 0.34)
        double arrowStep = (step > 0 ? step : std::abs(maxValue - minValue) / 100) * (maxValue >= minValue ? 1 : -1);
        double stepsCount = std::floor(arrowStepsAccumulated);
        if (stepsCount >= 1 && arrowStep != 0) {
            arrowStepsAccumulated -= stepsCount;
            double oldValue = value;
            double gridIndex = (value - minValue) / arrowStep;
            double newGridIndex = pressedArrow > 0 ? std::floor(gridIndex + 1e-6) + stepsCount : std::ceil(gridIndex - 1e-6) - stepsCount;
            setValue(minValue + newGridIndex * arrowStep);
            if (value != oldValue)
                em.emitEvent(ValueChanged_Event(this, value));
        }
    }

    AbstractWidget::render();
}







// Задать диапазон значений и шаг (0 - плавно); по умолчанию 0..1 плавно. Текущее значение обрезается под новый диапазон
void Slider::setRange(double min, double max, double step) {
    minValue = min;
    maxValue = max;
    this->step = step;
    setValue(value);
}




// Установить значение (в единицах диапазона): за краями обрезается, со шагом - снимается на ближайшее деление. Сигнал не шлёт
void Slider::setValue(double newValue) {
    newValue = std::clamp(newValue, (std::min)(minValue, maxValue), (std::max)(minValue, maxValue));
    if (step > 0)
        newValue = std::clamp(minValue + std::round((newValue - minValue) / step) * step, (std::min)(minValue, maxValue), (std::max)(minValue, maxValue));
    value = newValue;
    sliderPos = maxValue != minValue ? (value - minValue) / (maxValue - minValue) : 0;
    updateLater();
}












// Курсор покинул родительский виджет
void Slider::cursorLeftParentWidget() {
    if (isSliderPress || isCursorOnSlider || pressedArrow != 0 || hoveredArrow != 0) {
        isSliderPress = false;
        isCursorOnSlider = false;
        pressedArrow = 0;
        hoveredArrow = 0;
        updateTexture();
    }
}









// Создание текстуры кнопки
void Slider::updateTexture() {

    // Цвет элементов
    glm::vec3 imgColorFactor;
    if (isSliderPress || isBlocked)
        imgColorFactor = {0.5, 0.5, 0.5};
    else if (isCursorOnSlider)
        imgColorFactor = {0.8, 0.8, 0.8};
    else
        imgColorFactor = {1, 1, 1};

    // Данные текстуры
    auto &data = texture.data;
    std::ranges::fill(data, Pixel(0, 0));




    // Отрисовка линии (между стрелками, по ходу центра ползунка)
    int lineThickness = 6;
    texture.drawRectangle(widgetSize.height * 1.5, texture.height() / 2 - lineThickness / 2, widgetSize.width - widgetSize.height * 3, lineThickness, theme().sliderLine);


    // Отрисовка стрелок: треугольники остриём наружу посередине своих квадратов, края сглажены (4x4 выборки на пиксель)
    double arrowHeight = texture.height() * 0.6;
    double arrowWidth = arrowHeight * 0.88;
    double arrowCenterY = texture.height() / 2.0;
    for (int direction : {-1, 1}) {

        // Цвет стрелки
        float arrowColorFactor;
        if (pressedArrow == direction || isBlocked)
            arrowColorFactor = 0.5;
        else if (hoveredArrow == direction)
            arrowColorFactor = 0.8;
        else
            arrowColorFactor = 1;
        Pixel arrowColor = theme().sliderArrow * arrowColorFactor;

        // Середина квадрата стрелки и остриё
        double arrowCenterX = direction < 0 ? texture.height() / 2.0 : texture.width() - texture.height() / 2.0;
        double tipX = arrowCenterX + direction * arrowWidth / 2;

        for (int y = int(arrowCenterY - arrowHeight / 2); y <= int(arrowCenterY + arrowHeight / 2); ++y) {
            for (int x = int(arrowCenterX - arrowWidth / 2); x <= int(arrowCenterX + arrowWidth / 2); ++x) {

                // Сколько выборок пикселя внутри треугольника: полувысота растёт от острия к основанию
                int samplesInside = 0;
                for (int sampleY = 0; sampleY < 4; ++sampleY) {
                    for (int sampleX = 0; sampleX < 4; ++sampleX) {
                        double distanceFromTip = (tipX - (x + (sampleX + 0.5) / 4)) * direction;
                        double halfHeight = distanceFromTip / arrowWidth * arrowHeight / 2;
                        if (distanceFromTip >= 0 && distanceFromTip <= arrowWidth && std::abs(y + (sampleY + 0.5) / 4 - arrowCenterY) <= halfHeight)
                            ++samplesInside;
                    }
                }

                if (samplesInside > 0)
                    texture.setPixel({x, y}, Pixel(arrowColor.r, arrowColor.g, arrowColor.b, samplesInside * arrowColor.a / 16));
            }
        }
    }


    // Отрисовка ползунка
    texture.drawImage(sliderAbsPos(), L"IDR_CircleSlider", texture.height(), {-0.5, -0.5}, imgColorFactor);



    texture.update();
}






// Выдача абсолютной позиции слайдера относительно виджета (центр ходит между стрелками, отступив от них на радиус)
glm::ivec2 Slider::sliderAbsPos() {
    return glm::ivec2(widgetSize.height * 1.5 + sliderPos * (widgetSize.width - widgetSize.height * 3), widgetSize.height / 2);
}

void Slider::setSliderAbsPos(int absX) {
    // Вычисляем ширину области, по которой может двигаться слайдер (без квадратов стрелок)
    int range = widgetSize.width - widgetSize.height * 3;

    // Проверка, чтобы не делить на 0, если размер виджета как-то криво задан
    if (range <= 0) {
        sliderPos = 0.0f;
        return;
    }

    // Вычисляем относительную позицию слайдера в диапазоне [0.0, 1.0]
    double newPos = std::clamp(double(absX - widgetSize.height * 1.5) / double(range), 0.0, 1.0);

    // Позиция в значение диапазона, со шагом ползунок прыгает по делениям; сигнал - только если значение поменялось
    double oldValue = value;
    setValue(minValue + newPos * (maxValue - minValue));
    if (value != oldValue)
        em.emitEvent(ValueChanged_Event(this, value));
}









// Проверка нахождения курсора в пределах ползунка или стрелок
bool Slider::cursorInWidget() {
    bool result = glm::length(
                glm::vec2(guiContext->cursorPosition()) -
                glm::vec2(widgetPos) - glm::vec2(sliderAbsPos())) <=
            widgetSize.height / 2 || arrowUnderCursor() != 0;
    // Установка курсора
    if (result) {
        if (!isCustomSetCursor) {
            guiContext->setSystemCursor(cursorType);
        }
    }
    return result;
}




// Стрелка под курсором: -1 левая, 1 правая, 0 - курсор не над стрелкой
int Slider::arrowUnderCursor() {
    glm::dvec2 cursor = glm::dvec2(guiContext->cursorPosition()) - widgetPos;
    if (cursor.y < 0 || cursor.y >= widgetSize.height)
        return 0;
    if (cursor.x >= 0 && cursor.x < widgetSize.height)
        return -1;
    if (cursor.x < widgetSize.width && cursor.x >= widgetSize.width - widgetSize.height)
        return 1;
    return 0;
}


} // namespace vgui
