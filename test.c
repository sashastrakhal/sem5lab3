// Тестова програма, обчислення факторіала 5
int n = 5;
int fact = 1;

while (n > 1) {
    fact = fact * n;
    n = n - 1;
}

if (fact == 120) {
    print(fact);
} else {
    print(0);
}