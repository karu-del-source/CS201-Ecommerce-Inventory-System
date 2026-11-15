#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <cstdlib>
#include <cctype>
using namespace std;

#define NOCOPY(T) T() = default; T(const T&) = delete; T& operator=(const T&) = delete;

struct Product {
    int id = 0;            // Unique Identifier
    string name;           // Product Title
    string category;       // Category
    string brand;          // Official Partner Brand
    double price = 0;      // Unit Price (PHP)
    int stockQuantity = 0; // Available Inventory Level
};

// ---------------------------------------------------------------- input utils
struct Eof {};  // thrown when stdin closes so destructors still run
static string lower(string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }
static string line(const string& p) {
    cout << p; string s;
    if (!getline(cin, s)) throw Eof();
    size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}
static int readInt(const string& p, int lo, int hi) {
    for (;;) {
        string s = line(p);
        try { size_t pos; long v = stol(s, &pos); if (pos == s.size() && v >= lo && v <= hi) return (int)v; } catch (...) {}
        cout << "  Enter a whole number from " << lo << " to " << hi << ".\n";
    }
}
static double readDouble(const string& p, double lo, double hi) {
    for (;;) {
        string s = line(p);
        try { size_t pos; double v = stod(s, &pos); if (pos == s.size() && v >= lo && v <= hi) return v; } catch (...) {}
        cout << "  Enter an amount from " << lo << " to " << hi << ".\n";
    }
}
static string readText(const string& p) { string s; while ((s = line(p)).empty()) cout << "  Cannot be empty.\n"; return s; }
static string fit(const string& s, size_t w) { return s.size() < w ? s : s.substr(0, w - 1) + "."; }

// Clears the current terminal screen before showing another menu/page.
static void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[H";
#endif
}

// Keeps the current page visible until the user chooses to go back.
static void waitForBack() {
    line("\n0. Back (press Enter or type 0): ");
}

// ================================================ LAB 1: dynamic array catalog
class Catalog {
    Product* a = new Product[4]; int n = 0, cap = 4;
public:
    bool quiet = false;
    ~Catalog() { delete[] a; }
    NOCOPY(Catalog)
    int size() const { return n; }
    int capacity() const { return cap; }
    Product& at(int i) { return a[i]; }
    const Product& at(int i) const { return a[i]; }
    const Product* data() const { return a; }
    void insert(const Product& p) {            // keeps array ordered by id
        if (n == cap) {                        // grow: allocate, copy, free old
            Product* b = new Product[cap * 2];
            for (int i = 0; i < n; ++i) b[i] = a[i];
            delete[] a; a = b;
            if (!quiet) cout << "  [Lab 1] Catalog resized: " << cap << " -> " << cap * 2 << " slots\n";
            cap *= 2;
        }
        int i = n++;
        while (i > 0 && a[i - 1].id > p.id) { a[i] = a[i - 1]; --i; }
        a[i] = p;
    }
    void removeAt(int i) { for (; i < n - 1; ++i) a[i] = a[i + 1]; --n; }
    Product* copy() const { Product* c = new Product[n ? n : 1]; for (int i = 0; i < n; ++i) c[i] = a[i]; return c; }
};

// ============================================== LAB 6: BST, product id -> slot
struct TNode { int id, idx; TNode *l, *r; };
class Index {
    TNode* root = nullptr;
    static void kill(TNode* t) { if (t) { kill(t->l); kill(t->r); delete t; } }
    static TNode* build(const Catalog& c, int lo, int hi) {     // median-first = balanced tree
        if (lo > hi) return nullptr;
        int m = (lo + hi) / 2;
        TNode* t = new TNode{c.at(m).id, m, nullptr, nullptr};
        t->l = build(c, lo, m - 1); t->r = build(c, m + 1, hi);
        return t;
    }
    static int h(const TNode* t) { return t ? 1 + max(h(t->l), h(t->r)) : 0; }
public:
    ~Index() { kill(root); }
    NOCOPY(Index)
    void rebuild(const Catalog& c) { kill(root); root = build(c, 0, c.size() - 1); }
    int find(int id, int& steps) const {
        steps = 0;
        for (const TNode* t = root; t; t = id < t->id ? t->l : t->r) { ++steps; if (t->id == id) return t->idx; }
        return -1;
    }
    int height() const { return h(root); }
};

// ======================================== LAB 3: circular list of partner brands
struct BNode { string name, tagline; BNode* next; };
class BrandRing {
    BNode* tail = nullptr; int n = 0;       // tail->next is the first node
public:
    ~BrandRing() {
        if (!tail) return;
        BNode* c = tail->next; tail->next = nullptr;   // break the circle, then free
        while (c) { BNode* nx = c->next; delete c; c = nx; }
    }
    NOCOPY(BrandRing)
    void add(const string& name, const string& tag) {
        BNode* b = new BNode{name, tag, nullptr};
        if (!tail) b->next = b; else { b->next = tail->next; tail->next = b; }
        tail = b; ++n;
    }
    int size() const { return n; }
    BNode* first() const { return tail ? tail->next : nullptr; }
    BNode* at(int i) const { BNode* b = first(); while (i-- > 0) b = b->next; return b; }
};

// ======================================== LAB 2: doubly linked cart, singly wishlist
struct CNode { int id, qty; CNode *prev, *next; };
class Cart {
    CNode *hd = nullptr, *tl = nullptr, *cur = nullptr; int n = 0;
public:
    ~Cart() { clear(); }
    NOCOPY(Cart)
    CNode* find(int id) const { for (CNode* c = hd; c; c = c->next) if (c->id == id) return c; return nullptr; }
    int qtyOf(int id) const { CNode* f = find(id); return f ? f->qty : 0; }
    void add(int id, int qty) {
        if (CNode* f = find(id)) { f->qty += qty; cur = f; return; }
        CNode* nd = new CNode{id, qty, tl, nullptr};
        if (tl) tl->next = nd; else hd = nd;
        tl = cur = nd; ++n;
    }
    bool next() { if (cur && cur->next) { cur = cur->next; return true; } return false; }
    bool prev() { if (cur && cur->prev) { cur = cur->prev; return true; } return false; }
    void removeCurrent() {
        if (!cur) return;
        CNode* v = cur;
        if (v->prev) v->prev->next = v->next; else hd = v->next;
        if (v->next) v->next->prev = v->prev; else tl = v->prev;
        cur = v->next ? v->next : v->prev;
        delete v; --n;
    }
    void clear() { while (hd) { CNode* nx = hd->next; delete hd; hd = nx; } tl = cur = nullptr; n = 0; }
    CNode* head() const { return hd; }
    CNode* current() const { return cur; }
    int size() const { return n; }
};

struct WNode { int id; WNode* next; };
class Wishlist {
    WNode* hd = nullptr;
public:
    ~Wishlist() { while (hd) { WNode* nx = hd->next; delete hd; hd = nx; } }
    NOCOPY(Wishlist)
    bool contains(int id) const { for (WNode* w = hd; w; w = w->next) if (w->id == id) return true; return false; }
    bool add(int id) {
        if (contains(id)) return false;
        WNode* nd = new WNode{id, nullptr};
        if (!hd) hd = nd; else { WNode* w = hd; while (w->next) w = w->next; w->next = nd; }
        return true;
    }
    bool remove(int id) {
        WNode *p = nullptr, *w = hd;
        while (w && w->id != id) { p = w; w = w->next; }
        if (!w) return false;
        if (p) p->next = w->next; else hd = w->next;
        delete w; return true;
    }
    WNode* head() const { return hd; }
};

// ==================================================== LAB 5: FIFO order queue
struct Order { int no = 0; string cust; int n = 0; int* ids = nullptr; int* qty = nullptr; Order* next = nullptr; };
class OrderQueue {
    Order *f = nullptr, *r = nullptr; int cnt = 0, nextNo = 1001;
public:
    ~OrderQueue() { while (Order* o = pop()) release(o); }
    NOCOPY(OrderQueue)
    static void release(Order* o) { delete[] o->ids; delete[] o->qty; delete o; }
    int push(const string& cust, const Cart& c) {          // copies the cart's lines
        Order* o = new Order;
        o->no = nextNo++; o->cust = cust; o->n = c.size();
        o->ids = new int[o->n]; o->qty = new int[o->n];
        int i = 0;
        for (CNode* x = c.head(); x; x = x->next, ++i) { o->ids[i] = x->id; o->qty[i] = x->qty; }
        if (r) r->next = o; else f = o;
        r = o; ++cnt; return o->no;
    }
    Order* pop() {                                          // caller must release()
        Order* o = f; if (!o) return nullptr;
        f = o->next; if (!f) r = nullptr;
        o->next = nullptr; --cnt; return o;
    }
    const Order* front() const { return f; }
    int size() const { return cnt; }
};

// ======================================================= LAB 4: LIFO stack
enum Kind { ADD, REMOVE, STOCK, PRICE };
struct Action { Kind k = ADD; Product p; int delta = 0; double oldP = 0, newP = 0; };
struct SNode { Action a; SNode* next; };
class ActionStack {
    SNode* top = nullptr; int n = 0;
public:
    ~ActionStack() { clear(); }
    NOCOPY(ActionStack)
    void push(const Action& a) { top = new SNode{a, top}; ++n; }
    bool pop(Action& out) {
        if (!top) return false;
        SNode* t = top; out = t->a; top = t->next; delete t; --n; return true;
    }
    bool peek(Action& out) const { if (!top) return false; out = top->a; return true; }
    void clear() { Action tmp; while (pop(tmp)) {} }
    int size() const { return n; }
};

// ================================================= LAB 8: sorting algorithms
static long g_cmp = 0;
typedef bool (*Less)(const Product&, const Product&);
static bool byPrice(const Product& a, const Product& b) { ++g_cmp; return a.price < b.price; }
static bool byStock(const Product& a, const Product& b) { ++g_cmp; return a.stockQuantity < b.stockQuantity; }
static bool byBrand(const Product& a, const Product& b) { ++g_cmp; return a.brand != b.brand ? a.brand < b.brand : a.name < b.name; }

static void insertionSort(Product* a, int n, Less less) {
    for (int i = 1; i < n; ++i) {
        Product key = a[i]; int j = i - 1;
        while (j >= 0 && less(key, a[j])) { a[j + 1] = a[j]; --j; }
        a[j + 1] = key;
    }
}
static void quickSort(Product* a, int lo, int hi, Less less) {
    while (lo < hi) {
        swap(a[lo + (hi - lo) / 2], a[hi]);                 // middle pivot
        Product pv = a[hi]; int i = lo;
        for (int j = lo; j < hi; ++j) if (less(a[j], pv)) swap(a[i++], a[j]);
        swap(a[i], a[hi]);
        if (i - lo < hi - i) { quickSort(a, lo, i - 1, less); lo = i + 1; }   // recurse on smaller half
        else                 { quickSort(a, i + 1, hi, less); hi = i - 1; }
    }
}

// ============================================================ application state
struct Store {
    Catalog cat; Index idx; BrandRing ring; Cart cart; Wishlist wish; OrderQueue orders;
    ActionStack undo, redo; int nextId = 1;
};
static Product* lookup(Store& s, int id, int* st = nullptr) {
    int k, i = s.idx.find(id, k);
    if (st) *st = k;
    return i < 0 ? nullptr : &s.cat.at(i);
}
static int brandCount(const Store& s, const string& b) {
    int c = 0;
    for (int i = 0; i < s.cat.size(); ++i) if (s.cat.at(i).brand == b) ++c;
    return c;
}
static void row(const Product& p) {
    cout << left << setw(5) << p.id << setw(29) << fit(p.name, 28) << setw(13) << fit(p.category, 12)
         << setw(10) << fit(p.brand, 9) << right << setw(11) << p.price << setw(7) << p.stockQuantity << "\n";
}
static void table(const Product* a, int n) {
    if (!n) { cout << "  (nothing to show)\n"; return; }
    cout << left << setw(5) << "ID" << setw(29) << "Product" << setw(13) << "Category" << setw(10) << "Brand"
         << right << setw(11) << "Price(PHP)" << setw(7) << "Stock" << "\n" << string(75, '-') << "\n";
    for (int i = 0; i < n; ++i) row(a[i]);
    cout << string(75, '-') << "\n" << n << " product(s)\n";
}
static string pickBrand(const Store& s, bool allowName) {   // numbered list of partner brands
    for (int i = 0; i < s.ring.size(); ++i)
        cout << "  " << i + 1 << ". " << s.ring.at(i)->name << " (" << brandCount(s, s.ring.at(i)->name) << ")\n";
    string in = line(allowName ? "Brand (number or name): " : "Brand (number): ");
    for (int i = 0; i < s.ring.size(); ++i)
        if (in == to_string(i + 1) || (allowName && lower(in) == lower(s.ring.at(i)->name))) return s.ring.at(i)->name;
    return "";
}

static void seed(Store& s) {
    s.cat.quiet = true;
    const Product items[] = {
        {1, "iPhone 15 128GB", "Smartphones", "Apple", 55990, 12},   {2, "MacBook Air M2 13-inch", "Laptops", "Apple", 62990, 6},
        {3, "AirPods Pro (2nd Gen)", "Audio", "Apple", 14990, 25},   {4, "iPad 10th Gen 64GB", "Tablets", "Apple", 24990, 10},
        {5, "Apple Watch SE", "Wearables", "Apple", 15990, 14},      {6, "Galaxy S24 256GB", "Smartphones", "Samsung", 52990, 15},
        {7, "Galaxy Tab S9", "Tablets", "Samsung", 44990, 8},        {8, "Galaxy Buds2 Pro", "Audio", "Samsung", 9990, 30},
        {9, "Odyssey G5 27in Monitor", "Displays", "Samsung", 18990, 7}, {10, "Galaxy Watch6", "Wearables", "Samsung", 15490, 11},
        {11, "WH-1000XM5 Headphones", "Audio", "Sony", 21990, 18},   {12, "PlayStation 5 Slim", "Gaming", "Sony", 32990, 5},
        {13, "Xperia 5 V", "Smartphones", "Sony", 47990, 4},         {14, "Alpha A6400 Camera", "Cameras", "Sony", 58990, 3},
        {15, "WF-1000XM5 Earbuds", "Audio", "Sony", 16990, 20},      {16, "MX Master 3S Mouse", "Accessories", "Logitech", 5490, 40},
        {17, "MX Keys S Keyboard", "Accessories", "Logitech", 6290, 35}, {18, "C920 HD Pro Webcam", "Accessories", "Logitech", 3990, 28},
        {19, "G502 X Gaming Mouse", "Gaming", "Logitech", 4290, 32}, {20, "G733 Wireless Headset", "Gaming", "Logitech", 7990, 16}};
    for (const Product& p : items) s.cat.insert(p);
    s.nextId = 21; s.cat.quiet = false;
    s.ring.add("Apple", "Think different. Premium devices, seamless ecosystem.");
    s.ring.add("Samsung", "Do what you can't. Screens and smart devices for everyone.");
    s.ring.add("Sony", "Be moved. Studio-grade audio, cameras and gaming.");
    s.ring.add("Logitech", "Designed for play and productivity.");
    s.idx.rebuild(s.cat);
}

// ======================================================= menu 1-4: view/search
static void viewCatalog(Store& s) {
    clearScreen();
    cout << "\n=== COMPLETE CATALOG ===\n";
    table(s.cat.data(), s.cat.size());
    cout << "[Lab 1] capacity " << s.cat.capacity() << " | [Lab 6] BST height " << s.idx.height() << "\n";
    waitForBack();
}
static void filterBrand(Store& s) {
    clearScreen();
    string b = pickBrand(s, true);
    if (b.empty()) { cout << "  Not an official partner brand.\n"; waitForBack(); return; }
    int n = s.cat.size(), k = 0; Product* out = new Product[n ? n : 1];
    for (int i = 0; i < n; ++i) if (s.cat.at(i).brand == b) out[k++] = s.cat.at(i);
    table(out, k);
    delete[] out;
    waitForBack();
}
static bool has(const string& hay, const string& kw) { return lower(hay).find(kw) != string::npos; }
static void search(Store& s) {
    clearScreen();
    cout << "\n  1. Keyword (Linear Search)\n  2. Product ID (BST)\n  0. Back\n";
    int c = readInt("Choice: ", 0, 2);
    if (c == 0) return;
    clearScreen();
    if (c == 1) {
        string kw = lower(readText("Keyword (name, brand or category): "));
        int n = s.cat.size(), k = 0; Product* out = new Product[n ? n : 1];
        for (int i = 0; i < n; ++i) {
            const Product& p = s.cat.at(i);
            if (has(p.name, kw) || has(p.brand, kw) || has(p.category, kw)) out[k++] = p;
        }
        table(out, k);
        cout << "[Lab 7] linear search checked " << n << " record(s)\n";
        delete[] out;
    } else {
        int steps; Product* p = lookup(s, readInt("Product ID: ", 1, 2000000000), &steps);
        if (p) table(p, 1); else cout << "  No product with that ID.\n";
        cout << "[Lab 6] BST visited " << steps << " node(s)\n";
    }
    waitForBack();
}

static int bound(const Product* a, int n, double x, bool upper, int& steps) {  // binary search
    int lo = 0, hi = n;
    while (lo < hi) {
        ++steps; int m = (lo + hi) / 2;
        if (upper ? a[m].price <= x : a[m].price < x) lo = m + 1; else hi = m;
    }
    return lo;
}
static void priceRange(Store& s) {
    clearScreen();
    double lo = readDouble("Min price (PHP): ", 0, 1e9), hi = readDouble("Max price (PHP): ", 0, 1e9);
    if (lo > hi) { cout << "  Min cannot exceed max.\n"; waitForBack(); return; }
    int n = s.cat.size(), steps = 0; Product* a = s.cat.copy();
    quickSort(a, 0, n - 1, byPrice);                         // binary search needs sorted data
    int from = bound(a, n, lo, false, steps), to = bound(a, n, hi, true, steps);
    table(a + from, to - from);
    cout << "[Lab 7] binary search used " << steps << " step(s)\n";
    delete[] a;
    waitForBack();
}

// ================================================================ menu 5: cart
static bool addToCart(Store& s, int id, int qty) {
    Product* p = lookup(s, id);
    if (!p) { cout << "  No product with ID " << id << ".\n"; return false; }
    if (p->stockQuantity - s.cart.qtyOf(id) < qty) {
        cout << "  Not enough stock (" << p->stockQuantity << " available, " << s.cart.qtyOf(id) << " already in cart).\n";
        return false;
    }
    s.cart.add(id, qty); cout << "  Added " << qty << " x " << p->name << ".\n"; return true;
}
static void cartLine(Store& s, const CNode* c) {
    Product* p = lookup(s, c->id);
    cout << (c == s.cart.current() ? " > " : "   ") << left << setw(5) << c->id;
    if (p) cout << setw(29) << fit(p->name, 28) << right << "x" << c->qty << "  " << p->price * c->qty << "\n";
    else cout << "(no longer available)\n";
}
static void viewCart(Store& s) {
    if (!s.cart.head()) { cout << "  Cart is empty.\n"; return; }
    double total = 0;
    for (CNode* c = s.cart.head(); c; c = c->next) {
        cartLine(s, c);
        if (Product* p = lookup(s, c->id)) total += p->price * c->qty;
    }
    cout << "   Total: PHP " << total << "  ('>' = cursor)\n";
}
static void navigateCart(Store& s) {
    while (s.cart.current()) {
        clearScreen();
        cout << "--- CART NAVIGATION ---\n";
        cartLine(s, s.cart.current());
        string c = lower(line("[N]ext [P]rev [R]emove [0] Back > "));
        if (c == "n") { if (!s.cart.next()) cout << "  Last item.\n"; }
        else if (c == "p") { if (!s.cart.prev()) cout << "  First item.\n"; }
        else if (c == "r") s.cart.removeCurrent();
        else if (c == "0" || c == "b" || c == "q") return;
    }
    cout << "  Cart is empty.\n";
    waitForBack();
}
static void wishlist(Store& s) {
    for (;;) {
        clearScreen();
        cout << "\n--- WISHLIST (Singly Linked List) ---\n  1. Add  2. View  3. Remove  4. Move to cart  5. Back\n";
        int c = readInt("Choice: ", 1, 5);
        if (c == 5) return;
        if (c == 2) {
            if (!s.wish.head()) cout << "  Wishlist is empty.\n";
            for (WNode* w = s.wish.head(); w; w = w->next)
                if (Product* p = lookup(s, w->id)) row(*p); else cout << "  #" << w->id << " (unavailable)\n";
            waitForBack();
            continue;
        }
        int id = readInt("Product ID: ", 1, 2000000000); Product* p = lookup(s, id);
        if (c == 1) { if (!p) cout << "  No such product.\n"; else cout << (s.wish.add(id) ? "  Added.\n" : "  Already listed.\n"); }
        else if (c == 3) cout << (s.wish.remove(id) ? "  Removed.\n" : "  Not on wishlist.\n");
        else if (!s.wish.contains(id)) cout << "  Not on wishlist.\n";
        else if (!p) { s.wish.remove(id); cout << "  Product no longer exists; removed.\n"; }
        else if (addToCart(s, id, 1)) s.wish.remove(id);
        waitForBack();
    }
}
static void cartMenu(Store& s) {
    for (;;) {
        clearScreen();
        cout << "\n--- SHOPPING CART (Doubly Linked List) ---\n  1. Add item  2. View  3. Navigate  4. Wishlist  5. Clear  6. Back\n";
        int c = readInt("Choice: ", 1, 6);
        if (c == 6) return;
        if (c == 1) {
            int id = readInt("Product ID: ", 1, 2000000000);
            addToCart(s, id, readInt("Quantity: ", 1, 1000000));
        } else if (c == 2) viewCart(s);
        else if (c == 3) { if (s.cart.size()) navigateCart(s); else cout << "  Cart is empty.\n"; }
        else if (c == 4) wishlist(s);
        else { s.cart.clear(); cout << "  Cart cleared.\n"; }
        waitForBack();
    }
}

// ============================================================= menu 6: checkout
static void placeOrder(Store& s) {
    if (!s.cart.size()) { cout << "  Cart is empty.\n"; return; }
    for (CNode* c = s.cart.head(); c; c = c->next) {
        Product* p = lookup(s, c->id);
        if (!p || p->stockQuantity < c->qty) { cout << "  Item #" << c->id << " is unavailable or short on stock. Fix your cart.\n"; return; }
    }
    string name = line("Customer name (Enter = Guest): ");
    int no = s.orders.push(name.empty() ? "Guest" : name, s.cart);
    s.cart.clear();
    cout << "  Order #" << no << " queued. Position: " << s.orders.size() << ".\n";
}
static void processOrder(Store& s) {
    Order* o = s.orders.pop();                              // dequeue the front (FIFO)
    if (!o) { cout << "  Queue is empty.\n"; return; }
    cout << "\nProcessing order #" << o->no << " for " << o->cust << "\n";
    bool ok = true;
    for (int i = 0; i < o->n && ok; ++i) {                  // validate everything first
        Product* p = lookup(s, o->ids[i]);
        if (!p) { cout << "  REJECTED: product #" << o->ids[i] << " no longer exists.\n"; ok = false; }
        else if (p->stockQuantity < o->qty[i]) {
            cout << "  REJECTED: only " << p->stockQuantity << " of " << p->name << " left (needs " << o->qty[i] << ").\n"; ok = false;
        }
    }
    if (ok) {                                               // then commit
        double total = 0;
        for (int i = 0; i < o->n; ++i) {
            Product* p = lookup(s, o->ids[i]);
            p->stockQuantity -= o->qty[i]; total += p->price * o->qty[i];
            cout << "  " << fit(p->name, 28) << " x" << o->qty[i] << " (stock left " << p->stockQuantity << ")\n";
        }
        cout << "  Fulfilled. Total: PHP " << total << "\n";
    }
    OrderQueue::release(o);
}
static void checkoutMenu(Store& s) {
    for (;;) {
        clearScreen();
        cout << "\n--- CHECKOUT QUEUE (FIFO) --- waiting: " << s.orders.size()
             << "\n  1. Place order  2. Process next  3. Process all  4. View queue  5. Back\n";
        int c = readInt("Choice: ", 1, 5);
        if (c == 5) return;
        if (c == 1) placeOrder(s);
        else if (c == 2) processOrder(s);
        else if (c == 3) { if (!s.orders.size()) cout << "  Queue is empty.\n"; while (s.orders.size()) processOrder(s); }
        else {
            int pos = 1;
            if (!s.orders.front()) cout << "  Queue is empty.\n";
            for (const Order* o = s.orders.front(); o; o = o->next, ++pos)
                cout << "  " << pos << ". #" << o->no << " " << o->cust << " (" << o->n << " line(s))\n";
        }
        waitForBack();
    }
}

// ========================================================== menu 7: admin + undo
static string describe(const Action& a) {
    string who = "#" + to_string(a.p.id) + " " + a.p.name;
    switch (a.k) {
        case ADD: return "Added " + who;
        case REMOVE: return "Removed " + who;
        case STOCK: return "Stock " + string(a.delta > 0 ? "+" : "") + to_string(a.delta) + " on " + who;
        default: return "Price change on " + who;
    }
}
static bool apply(Store& s, const Action& a, bool undoing) {    // forward (redo) or backward (undo)
    Product* p = lookup(s, a.p.id);
    if (a.k == ADD || a.k == REMOVE) {
        bool exists = (a.k == ADD) ? !undoing : undoing;        // should it exist afterwards?
        if (exists && !p) s.cat.insert(a.p);
        else if (!exists && p) { int st; s.cat.removeAt(s.idx.find(a.p.id, st)); }
        else { cout << "  Refused: catalog no longer matches this action.\n"; return false; }
        s.idx.rebuild(s.cat);                                   // slots shifted: refresh BST
        return true;
    }
    if (!p) { cout << "  Refused: product not found.\n"; return false; }
    if (a.k == STOCK) {
        int d = undoing ? -a.delta : a.delta;
        if (p->stockQuantity + d < 0) { cout << "  Refused: units were already sold.\n"; return false; }
        p->stockQuantity += d;
    } else p->price = undoing ? a.oldP : a.newP;
    return true;
}
static void record(Store& s, const Action& a) { apply(s, a, false); s.undo.push(a); s.redo.clear(); }

static void adminMenu(Store& s) {
    for (;;) {
        clearScreen();
        Action top;
        cout << "\n--- ADMIN PANEL (LIFO Undo/Redo) --- undo: " << s.undo.size() << " redo: " << s.redo.size();
        if (s.undo.peek(top)) cout << "\n  Last: " << describe(top);
        cout << "\n  1. Adjust stock  2. Update price  3. Add product  4. Remove product  5. UNDO  6. Redo  7. Back\n";
        int c = readInt("Choice: ", 1, 7);
        if (c == 7) return;
        Action a;
        if (c == 5 || c == 6) {
            ActionStack& from = (c == 5) ? s.undo : s.redo; ActionStack& to = (c == 5) ? s.redo : s.undo;
            if (!from.pop(a)) { cout << "  Nothing to " << (c == 5 ? "undo" : "redo") << ".\n"; waitForBack(); continue; }
            if (apply(s, a, c == 5)) { to.push(a); cout << (c == 5 ? "  Undone: " : "  Redone: ") << describe(a) << "\n"; } else from.push(a);
            waitForBack();
            continue;
        }
        if (c == 3) {
            a.k = ADD; a.p.name = readText("Name: "); a.p.category = readText("Category: ");
            a.p.brand = pickBrand(s, false);
            if (a.p.brand.empty()) { cout << "  Pick an official partner brand.\n"; waitForBack(); continue; }
            a.p.price = readDouble("Price: ", 0.01, 1e9); a.p.stockQuantity = readInt("Stock: ", 0, 1000000);
            a.p.id = s.nextId++; record(s, a); cout << "  Added as #" << a.p.id << ".\n"; waitForBack(); continue;
        }
        Product* p = lookup(s, readInt("Product ID: ", 1, 2000000000));
        if (!p) { cout << "  No such product.\n"; waitForBack(); continue; }
        a.p = *p;
        if (c == 1) {
            a.k = STOCK; a.delta = readInt("Change in units (+/-): ", -1000000, 1000000);
            if (p->stockQuantity + a.delta < 0) { cout << "  Stock cannot go below zero.\n"; waitForBack(); continue; }
        } else if (c == 2) { a.k = PRICE; a.oldP = p->price; a.newP = readDouble("New price: ", 0.01, 1e9); }
        else { a.k = REMOVE; if (line("Remove " + p->name + "? (y/n): ") != "y") { waitForBack(); continue; } }
        record(s, a); cout << "  Done: " << describe(a) << "\n";   // 'p' may be stale after REMOVE; not used again
        waitForBack();
    }
}

// ============================================================ menu 8 and 9
static void showcase(Store& s) {
    BNode* b = s.ring.first(); int pos = 1;
    for (;;) {
        clearScreen();
        cout << "\n+-- FEATURED PARTNER " << pos << "/" << s.ring.size() << ": " << b->name << " --+\n  " << b->tagline
             << "\n  " << brandCount(s, b->name) << " product(s) listed\n";
        string choice = lower(line("[Enter] next banner, [0] Back > "));
        if (choice == "0" || choice == "b" || choice == "q") return;
        b = b->next;                                        // circular: last->next is the first
        if (++pos > s.ring.size()) { pos = 1; cout << "  ...wrapped around to the first brand...\n"; }
    }
}
static void sortMenu(Store& s) {
    clearScreen();
    cout << "  Sort by: 1. Price  2. Stock  3. Brand  0. Back\n";
    int key = readInt("Choice: ", 0, 3);
    if (key == 0) return;
    clearScreen();
    cout << "  Algorithm: 1. Insertion  2. Quick  0. Back\n";
    int alg = readInt("Choice: ", 0, 2);
    if (alg == 0) return;
    clearScreen();
    cout << "  Order: 1. Ascending  2. Descending  0. Back\n";
    int ord = readInt("Choice: ", 0, 2);
    if (ord == 0) return;
    clearScreen();
    int n = s.cat.size(); Product* a = s.cat.copy();
    Less less = key == 1 ? byPrice : key == 2 ? byStock : byBrand;
    g_cmp = 0;
    if (alg == 1) insertionSort(a, n, less); else quickSort(a, 0, n - 1, less);
    if (ord == 2) for (int i = 0, j = n - 1; i < j; ++i, --j) swap(a[i], a[j]);
    table(a, n);
    cout << "[Lab 8] " << (alg == 1 ? "Insertion" : "Quick") << " sort: " << g_cmp << " comparisons for " << n << " items\n";
    delete[] a;
    waitForBack();
}

// ===================================================================== main
int main() {
    Store s; seed(s);
    cout << fixed << setprecision(2);
    try {
        for (;;) {
            clearScreen();
            cout << "\n==============================================================\n"
                    "  BRAND-AFFILIATED E-COMMERCE INVENTORY & SEARCH SYSTEM (CS201)\n"
                    "==============================================================\n"
                    "1. View Complete Catalog (Tabular View with Brand Affiliations)\n"
                    "2. Filter Products by Official Brand (Apple, Samsung, Sony, Logitech)\n"
                    "3. Search Products (Keyword: Linear Search | ID: BST)\n"
                    "4. Filter Products by Price Range (Binary Search)\n"
                    "5. Shopping Cart & Wishlist (Linked List Navigation)\n"
                    "6. Process Customer Checkout Queue (FIFO Order Queue)\n"
                    "7. Admin Panel / Perform Undo Action (LIFO Stack)\n"
                    "8. Display Partner Brand Banner Showcase (Circular List Cycle)\n"
                    "9. Sort Inventory Display (By Price, Stock, or Brand)\n"
                    "10. Exit\n"
                    "==============================================================\n";
            int c = readInt("Enter choice (1-10): ", 1, 10);
            if (c == 10) break;
            switch (c) {
                case 1: viewCatalog(s); break;   case 2: filterBrand(s); break;
                case 3: search(s); break;        case 4: priceRange(s); break;
                case 5: cartMenu(s); break;      case 6: checkoutMenu(s); break;
                case 7: adminMenu(s); break;     case 8: showcase(s); break;
                case 9: sortMenu(s); break;
            }
        }
        cout << "Goodbye!\n";
    } catch (const Eof&) { cout << "\nInput closed. Exiting.\n"; }
    return 0;                                               // Store destructors free every node
}