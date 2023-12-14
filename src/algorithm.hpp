#pragma once

#include <algorithm>
#include <cassert>

template<typename Container, typename Value>
void fill(Container& container, Value value)
{
    using namespace std;
    fill(begin(container), end(container), value);
}

template<typename ContainerIn, typename ContainerOut>
void copy(const ContainerIn& in, ContainerOut& out)
{
    using namespace std;
    assert(in.size() == out.size());
    copy(begin(in), end(in), begin(out));
}

template<typename Container, typename Predicate>
typename Container::iterator remove_if(
    Container& container, Predicate predicate
) {
    using namespace std;
    return remove_if(begin(container), end(container), predicate);
}

template<typename Container, typename Predicate>
void erase_if(Container& container, Predicate predicate)
{
    container.erase(remove_if(container, predicate), container.end());
}

template<typename Container>
void appendAll(Container& head, const Container& tail)
{
    head.insert(head.end(), tail.begin(), tail.end());
}

template<typename Container, typename Predicate>
auto find_if(const Container& container, Predicate predicate)
{
    using namespace std;
    return find_if(begin(container), end(container), predicate);
}
