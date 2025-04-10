#include <iostream>
#include <cstring>

const char* PUSH = "push";
const char* POP = "pop";
const char* BACK = "back";
const char* SIZE = "size";
const char* CLEAR = "clear";
const char* EXIT = "exit";

void Push(char**& stack, size_t& capacity, size_t& count_elems);

void Pop(char**& stack, size_t& count_elems);

void Back(char**& stack, size_t& count_elems);

void Clear(char**& stack, size_t& count_elems);

int main() {
    size_t capacity = 8;
    size_t count_elems = 0;
    char** stack =  (char**)calloc(8, sizeof(char*));
    char* command = (char*)calloc(8, sizeof(char));
    while (true) {
        scanf("%s", command);
        if (strcmp(command, PUSH) == 0) {
            Push(stack, capacity, count_elems);
        } else if (strcmp(command, POP) == 0) {
            Pop(stack, count_elems);
        } else if (strcmp(command, BACK) == 0) {
            Back(stack, count_elems);
        } else if (strcmp(command, SIZE) == 0) {
            std::cout << count_elems << std::endl;
        } else if (strcmp(command, CLEAR) == 0) {
            Clear(stack, count_elems);
            std::cout << "ok" << std::endl;        
        } else if (strcmp(command, EXIT) == 0) {
            std::cout << "bye" << std::endl;
            Clear(stack, count_elems);
            free(stack);
            free(command);
            break;
        }
    }
}

void Push(char**& stack, size_t& capacity, size_t& count_elems) {
    if (count_elems == capacity) {
        capacity *= 2;
        stack = (char**)realloc(stack, capacity * sizeof(char*));
    }
    char* new_string = NULL;
    scanf("%ms", &new_string);
    count_elems++;
    stack[count_elems - 1] = new_string;
    std::cout << "ok" << std::endl;
}

void Pop(char**& stack, size_t& count_elems) {
    if (count_elems == 0) {
        std::cout << "error" << std::endl;
    } else {
        printf("%s", stack[count_elems - 1]);
        free(stack[count_elems - 1]);
        std::cout << std::endl;
        count_elems--;
    }
}

void Back(char**& stack, size_t& count_elems) {
    if (count_elems == 0) {
        std::cout << "error" << std::endl;
    } else {
        printf("%s", stack[count_elems - 1]);
        std::cout << std::endl;
    }
}

void Clear(char**& stack, size_t& count_elems) {
    while (count_elems != 0) {
        free(stack[count_elems - 1]);
        count_elems--;
    }
}