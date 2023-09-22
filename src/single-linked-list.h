#pragma once

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <new>

template <typename Type>
class SingleLinkedList {
    // Узел списка
    struct Node {
        Node() = default;
        Node(const Type& val, Node* next)
            : value(val)
            , next_node(next) {
        }
        Type value;
        Node* next_node = nullptr;
    };

    template <typename ValueType>
    class BasicIterator {
        // Класс списка объявляется дружественным, чтобы из методов списка
        // был доступ к приватной области итератора
        friend class SingleLinkedList;

        // Конвертирующий конструктор итератора из указателя на узел списка
        explicit BasicIterator(Node* node) {
            node_ = node;
        }

    public:
        // Объявленные ниже типы сообщают стандартной библиотеке о свойствах этого итератора

        // Категория итератора — forward iterator
        // (итератор, который поддерживает операции инкремента и многократное разыменование)
        using iterator_category = std::forward_iterator_tag;
        // Тип элементов, по которым перемещается итератор
        using value_type = Type;
        // Тип, используемый для хранения смещения между итераторами
        using difference_type = std::ptrdiff_t;
        // Тип указателя на итерируемое значение
        using pointer = ValueType*;
        // Тип ссылки на итерируемое значение
        using reference = ValueType&;

        BasicIterator() = default;

        // Конвертирующий конструктор/конструктор копирования
        // При ValueType, совпадающем с Type, играет роль копирующего конструктора
        // При ValueType, совпадающем с const Type, играет роль конвертирующего конструктора
        BasicIterator(const BasicIterator<Type>& other) noexcept {
           node_ = other.node_;
        }

        // Чтобы компилятор не выдавал предупреждение об отсутствии оператора = при наличии
        // пользовательского конструктора копирования, явно объявим оператор = и
        // попросим компилятор сгенерировать его за нас
        BasicIterator& operator=(const BasicIterator& rhs) = default;

        template<typename T>
        [[nodiscard]] bool operator==(const T& rhs) const noexcept {
            return this->node_ == rhs.node_ ;
        }

        // Оператор проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<const Type>& rhs) const noexcept {
            return !(*this == rhs);
        }

        // Оператор проверки итераторов на неравенство
        // Противоположен !=
        [[nodiscard]] bool operator!=(const BasicIterator<Type>& rhs) const noexcept {
            return this->node_ != rhs.node_;
        }

        // Оператор прединкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает ссылку на самого себя
        // Инкремент итератора, не указывающего на существующий элемент списка, приводит к неопределённому поведению
        BasicIterator& operator++() noexcept {
            assert(node_);
            node_ = node_->next_node;
            return *this;
        }

        // Оператор постинкремента. После его вызова итератор указывает на следующий элемент списка
        // Возвращает прежнее значение итератора
        // Инкремент итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        BasicIterator operator++(int) noexcept {
            assert(node_);
            auto buff(*this); // используем конструктор копирования класса для инициализации объекта
            ++(*this);
            return buff;
        }

        // Операция разыменования. Возвращает ссылку на текущий элемент
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] reference operator*() const noexcept {
            assert(node_);
            return node_->value;
        }

        // Операция доступа к члену класса. Возвращает указатель на текущий элемент списка
        // Вызов этого оператора у итератора, не указывающего на существующий элемент списка,
        // приводит к неопределённому поведению
        [[nodiscard]] pointer operator->() const noexcept {
            assert(node_);
            return &node_->value; // -> по сути, передает ссылку, а ссылка является указателем, который сразу разымиовывается при обращении к нему
        }

    private:
        Node* node_ = nullptr;
    };

public:

    using value_type = Type;
    using reference = value_type&;
    using const_reference = const value_type&;

    // Итератор, допускающий изменение элементов списка
    using Iterator = BasicIterator<Type>;
    // Константный итератор, предоставляющий доступ для чтения к элементам списка
    using ConstIterator = BasicIterator<const Type>;


    SingleLinkedList() : head_(Node()), size_(0) {}

    SingleLinkedList(const SingleLinkedList<Type>& input) : head_(Node()), size_(0) {
        CopyList(input);
    }

    SingleLinkedList(std::initializer_list<Type> input) : head_(Node()), size_(0){ // !я создаю свой список из initializer_list только в этом конструкторе, а мой метод CopyList реализует копирование из другого объекта этого класса где я использую методы которые есть только в моем классе
        for (auto i = std::rbegin(input); i != std::rend(input); ++i) {
            this->PushFront(*i);
        }
    }

    ~SingleLinkedList() {
        this->Clear();
    }

    // Возвращает количество элементов в списке за время O(1)
    [[nodiscard]] size_t GetSize() const noexcept {
        return size_;
    }

    // Сообщает, пустой ли список за время O(1)
    [[nodiscard]] bool IsEmpty() const noexcept {
       return !(size_);
    }

    // Вставляет элемент value в начало списка за время O(1)
    void PushFront(const Type& value) {
        ConstIterator head(&head_);
        this->InsertAfter(head, value);
    }

    void PopFront() noexcept {
        assert(head_.next_node);
        ConstIterator head(&head_);
        this->EraseAfter(head);
    }

    /*
    * Вставляет элемент value после элемента, на который указывает pos.
    * Возвращает итератор на вставленный элемент
    * Если при создании элемента будет выброшено исключение, список останется в прежнем состоянии
     */
    Iterator InsertAfter(ConstIterator pos, const Type& value) {
        assert(pos.node_);
        Node* buff = new Node(value, pos.node_->next_node);
        pos.node_->next_node = buff;
        ++size_;
        return Iterator{buff};
    }

    /*
    * Удаляет элемент, следующий за pos.
    * Возвращает итератор на элемент, следующий за удалённым
    */
    Iterator EraseAfter(ConstIterator pos) noexcept {
        assert(pos.node_);
        delete std::exchange(pos.node_->next_node, pos.node_->next_node->next_node);
        --size_;
        return Iterator{pos.node_->next_node};
    }


    void swap(SingleLinkedList& other) noexcept {
        std::swap(this->size_, other.size_);
        std::swap(this->head_.next_node, other.head_.next_node);
    }

    // Очищает список за время O(N)
    void Clear() noexcept {
        while (!this->IsEmpty()) {
            this->PopFront();
        }
        size_ = 0;
    }

    // Возвращает итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    [[nodiscard]] Iterator begin() noexcept {
        return Iterator{head_.next_node};
    }

    // Возвращает итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] Iterator end() noexcept {
        return BasicIterator<Type>();
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен end()
    // Результат вызова эквивалентен вызову метода cbegin()
    [[nodiscard]] ConstIterator begin() const noexcept {
        return Iterator{head_.next_node};
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    // Результат вызова эквивалентен вызову метода cend()
    [[nodiscard]] ConstIterator end() const noexcept {
        return BasicIterator<Type>();
    }

    // Возвращает константный итератор, ссылающийся на первый элемент
    // Если список пустой, возвращённый итератор будет равен cend()
    [[nodiscard]] ConstIterator cbegin() const noexcept {
        return Iterator{head_.next_node};
    }

    // Возвращает константный итератор, указывающий на позицию, следующую за последним элементом односвязного списка
    // Разыменовывать этот итератор нельзя — попытка разыменования приведёт к неопределённому поведению
    [[nodiscard]] ConstIterator cend() const noexcept {
        return BasicIterator<Type>();
    }

    [[nodiscard]] Iterator before_begin() noexcept {
        return Iterator{&head_};
    }

    [[nodiscard]] ConstIterator cbefore_begin() const noexcept {
        return  Iterator{const_cast<Node*>(&head_)};
    }

    [[nodiscard]] ConstIterator before_begin() const noexcept{
        return  Iterator{const_cast<Node*>(&head_)};
    }

    [[nodiscard]] bool operator< (const SingleLinkedList<Type>& rhs) const {
        return std::lexicographical_compare(this->begin(), this->end(), rhs.begin(), rhs.end());
    }

    [[nodiscard]] bool operator== (const SingleLinkedList<Type>& rhs) const {
        return (this->GetSize() == rhs.GetSize() && std::equal(this->begin(), this->end(), rhs.begin(), rhs.end()));
    }

    [[nodiscard]] bool operator> (const SingleLinkedList<Type>& rhs) const {
        return rhs < *this;
    }

    [[nodiscard]] bool operator!= (const SingleLinkedList<Type>& rhs) const {
        return !(*this == rhs);
    }

    [[nodiscard]] bool operator>= (const SingleLinkedList<Type>& rhs) const {
        return !(*this < rhs);
    }

    [[nodiscard]] bool operator<= (const SingleLinkedList<Type>& rhs) const {
        return !(rhs < *this);
    }

    SingleLinkedList<Type>& operator= (const SingleLinkedList<Type>& rhs) {
        if (this == &rhs) return *this;
        return CopyList(rhs); // !copy and swap я реализовал в CopyList, может как то не так, не пойму, зачем мне тут дублировать метод
    }

private:
    
    SingleLinkedList<Type>& CopyList(const SingleLinkedList<Type>& input) {
        if (input.IsEmpty()) {
            this->Clear();
            return *this;
        }
        {
            SingleLinkedList<Type> buff;
            ConstIterator current_node_to_mod(&buff.head_);
            for (auto i = input.begin(); i != input.end(); ++i){
                current_node_to_mod = buff.InsertAfter(current_node_to_mod, i.node_->value);
            }
            buff.size_ = input.GetSize();
            this->swap(buff);
        }
        return *this;
    }

    // Фиктивный узел, используется для вставки "перед первым элементом"
    Node head_;
    size_t size_;
};

template <typename Type>
void swap(SingleLinkedList<Type>& lhs, SingleLinkedList<Type>& rhs) noexcept {
    lhs.swap(rhs);
}