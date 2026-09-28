#include "smart_ptr/SharedPtr.hpp"

#include <clocale>
#include <iostream>
#include <utility>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

struct Probe {
    static int alive;
    static int destructions;

    int value = 1;

    Probe() { ++alive; }

    ~Probe() {
        --alive;
        ++destructions;
    }

    void bump() { ++value; }
};

int Probe::alive = 0;
int Probe::destructions = 0;

void testOwnership() {
    Probe::alive = 0;
    Probe::destructions = 0;
    {
        mafia::SharedPtr<Probe> owner(new Probe());
        expect(Probe::alive == 1, "объект создан один раз");
        {
            mafia::SharedPtr<Probe> firstCopy = owner;
            mafia::SharedPtr<Probe> secondCopy = firstCopy;
            expect(owner.get() == firstCopy.get() && firstCopy.get() == secondCopy.get(),
                   "копии смотрят на один адрес");
            expect(Probe::alive == 1, "копия не создаёт второй объект");
            expect(Probe::destructions == 0, "деструктор не вызывается, пока есть копии");
        }
        expect(Probe::alive == 1, "объект жив, пока жива последняя копия");
    }
    expect(Probe::alive == 0, "объект удалён после последней копии");
    expect(Probe::destructions == 1, "деструктор вызвался один раз");
}

void testMove() {
    Probe::alive = 0;
    mafia::SharedPtr<Probe> source(new Probe());
    Probe* raw = source.get();
    mafia::SharedPtr<Probe> destination(std::move(source));
    expect(source.get() == nullptr, "перемещение опустошает источник");
    expect(source == nullptr, "источник после перемещения равен nullptr");
    expect(destination.get() == raw, "перемещение забирает тот же адрес");
    expect(Probe::alive == 1, "перемещение не удаляет объект");
    destination.reset();
    expect(Probe::alive == 0, "объект удалён после reset единственного владельца");
}

void testReset() {
    Probe::alive = 0;
    mafia::SharedPtr<Probe> pointer(new Probe());
    mafia::SharedPtr<Probe> copy = pointer;

    pointer.reset();
    expect(pointer.get() == nullptr, "reset опустошает указатель");
    expect(pointer == nullptr, "после reset указатель равен nullptr");
    expect(copy.get() != nullptr && Probe::alive == 1, "reset одной копии оставляет объект другим");

    copy.reset();
    expect(Probe::alive == 0, "reset последней копии удаляет объект");

    pointer.reset(new Probe());
    expect(pointer != nullptr && Probe::alive == 1, "reset принимает новый объект");
    pointer.reset();
}

void testSwap() {
    mafia::SharedPtr<Probe> left(new Probe());
    mafia::SharedPtr<Probe> right(new Probe());
    left->value = 10;
    right->value = 20;
    Probe* leftRaw = left.get();
    Probe* rightRaw = right.get();

    left.swap(right);

    expect(left.get() == rightRaw && right.get() == leftRaw, "swap меняет адреса");
    expect(left->value == 20 && (*right).value == 10, "swap меняет объекты местами");
    expect(left != right, "после swap указатели остаются разными");

    left.reset();
    right.reset();
}

void testCompare() {
    mafia::SharedPtr<Probe> empty;
    mafia::SharedPtr<Probe> alsoEmpty(nullptr);
    expect(empty == nullptr, "пустой равен nullptr");
    expect(nullptr == alsoEmpty, "nullptr равен пустому");
    expect(empty == alsoEmpty, "два пустых равны");
    expect(!(empty != nullptr), "пустой не отличается от nullptr");

    mafia::SharedPtr<Probe> first(new Probe());
    mafia::SharedPtr<Probe> copy = first;
    mafia::SharedPtr<Probe> second(new Probe());
    expect(first == copy, "копии равны");
    expect(first != second, "разные объекты не равны");
    expect(first != nullptr && nullptr != second, "непустой не равен nullptr");

    first.reset();
    copy.reset();
    second.reset();
}

void testDereferenceAndAssign() {
    Probe::alive = 0;
    Probe::destructions = 0;

    mafia::SharedPtr<Probe> first(new Probe());
    (*first).value = 5;
    first->bump();
    expect(first->value == 6, "разыменование * и -> работает");

    mafia::SharedPtr<Probe> second(new Probe());
    second = first;
    expect(Probe::destructions == 1, "присваивание удаляет прежний объект");
    expect(second == first && second->value == 6, "присваивание делит владение");
    expect(Probe::alive == 1, "после присваивания остаётся один объект");

    first = first;
    expect(first->value == 6 && Probe::alive == 1, "присваивание себе не удаляет объект");

    first.reset();
    second.reset();
    expect(Probe::alive == 0, "оба владельца отпустили объект");
}

}  // namespace

int main() {
#ifdef _WIN32
    // Строки теста в UTF-8, консоль Windows по умолчанию в OEM (866).
    std::setlocale(LC_CTYPE, ".UTF-8");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    testOwnership();
    testMove();
    testReset();
    testSwap();
    testCompare();
    testDereferenceAndAssign();

    if (failures != 0) {
        std::cerr << failures << " проверок не прошли\n";
        return 1;
    }
    std::cout << "SharedPtr: все проверки прошли\n";
    return 0;
}
