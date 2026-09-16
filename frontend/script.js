// ============================================================
// KaviyaMart - Main JavaScript
// ============================================================

// ------------------------------------------------------------
// BASIC HELPERS
// ------------------------------------------------------------

function getUserId() {
    return localStorage.getItem("user_id");
}

function getUserRole() {
    return localStorage.getItem("user_role");
}

function getUserName() {
    return localStorage.getItem("user_name") || "User";
}

function isLoggedIn() {
    return !!getUserId();
}

function showMessage(message, type = "success") {
    const existing = document.querySelector(".js-message");

    if (existing) {
        existing.remove();
    }

    const box = document.createElement("div");
    box.className = `js-message ${type === "error" ? "error-message" : "success-message"}`;
    box.textContent = message;

    box.style.position = "fixed";
    box.style.top = "90px";
    box.style.right = "20px";
    box.style.zIndex = "9999";
    box.style.maxWidth = "350px";
    box.style.padding = "14px 18px";
    box.style.borderRadius = "10px";
    box.style.boxShadow = "0 8px 25px rgba(0,0,0,0.15)";

    document.body.appendChild(box);

    setTimeout(() => {
        box.remove();
    }, 3000);
}


// ------------------------------------------------------------
// LOGOUT
// ------------------------------------------------------------

function logout() {
    localStorage.removeItem("user_id");
    localStorage.removeItem("user_name");
    localStorage.removeItem("user_email");
    localStorage.removeItem("user_role");

    sessionStorage.removeItem("kaviyaMartBuyNow");

    window.location.href = "index.html";
}


// ------------------------------------------------------------
// LOGIN
// ------------------------------------------------------------

async function handleLogin(event) {
    event.preventDefault();

    const emailInput = document.getElementById("loginEmail");
    const passwordInput = document.getElementById("loginPassword");

    if (!emailInput || !passwordInput) {
        return;
    }

    const email = emailInput.value.trim();
    const password = passwordInput.value;

    if (!email || !password) {
        showMessage("Please enter email and password.", "error");
        return;
    }

    try {
        const response = await fetch("/api/login", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                email: email,
                password: password
            })
        });

        const result = await response.json();

        if (!response.ok) {
            showMessage(
                result.message || result.error || "Login failed.",
                "error"
            );
            return;
        }

        // Store logged-in user information
        if (result.user_id !== undefined) {
            localStorage.setItem("user_id", result.user_id);
        }

        if (result.name) {
            localStorage.setItem("user_name", result.name);
        }

        if (result.email) {
            localStorage.setItem("user_email", result.email);
        }

        if (result.role) {
            localStorage.setItem("user_role", result.role);
        }

        // Some backend versions may return user object
        if (result.user) {
            if (result.user.user_id !== undefined) {
                localStorage.setItem("user_id", result.user.user_id);
            }

            if (result.user.name) {
                localStorage.setItem("user_name", result.user.name);
            }

            if (result.user.email) {
                localStorage.setItem("user_email", result.user.email);
            }

            if (result.user.role) {
                localStorage.setItem("user_role", result.user.role);
            }
        }

        const role = localStorage.getItem("user_role");

        showMessage("Login successful!");

        setTimeout(() => {
            if (role === "seller") {
                window.location.href = "seller.html";
            } else {
                window.location.href = "buyer.html";
            }
        }, 500);

    } catch (error) {
        console.error("Login error:", error);
        showMessage(
            "Unable to connect to KaviyaMart server.",
            "error"
        );
    }
}


// ------------------------------------------------------------
// REGISTER
// ------------------------------------------------------------

async function handleRegister(event) {
    event.preventDefault();

    const nameInput = document.getElementById("registerName");
    const emailInput = document.getElementById("registerEmail");
    const mobileInput = document.getElementById("registerMobile");
    const passwordInput = document.getElementById("registerPassword");
    const roleInput = document.getElementById("registerRole");

    if (
        !nameInput ||
        !emailInput ||
        !mobileInput ||
        !passwordInput ||
        !roleInput
    ) {
        return;
    }

    const name = nameInput.value.trim();
    const email = emailInput.value.trim();
    const mobile = mobileInput.value.trim();
    const password = passwordInput.value;
    const role = roleInput.value;

    if (!name || !email || !mobile || !password || !role) {
        showMessage("Please fill in all fields.", "error");
        return;
    }

    try {
        const response = await fetch("/api/register", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                name: name,
                email: email,
                mobile: mobile,
                password: password,
                role: role
            })
        });

        const result = await response.json();

        if (!response.ok) {
            showMessage(
                result.message || result.error || "Registration failed.",
                "error"
            );
            return;
        }

        showMessage(
            "Registration successful! Please login."
        );

        setTimeout(() => {
            window.location.href = "login.html";
        }, 1000);

    } catch (error) {
        console.error("Registration error:", error);

        showMessage(
            "Unable to connect to KaviyaMart server.",
            "error"
        );
    }
}


// ------------------------------------------------------------
// AUTH PROTECTION
// ------------------------------------------------------------

function requireLogin() {
    if (!isLoggedIn()) {
        window.location.href = "login.html";
        return false;
    }

    return true;
}

function requireBuyer() {
    if (!requireLogin()) {
        return false;
    }

    const role = getUserRole();

    if (role !== "buyer") {
        window.location.href = "seller.html";
        return false;
    }

    return true;
}

function requireSeller() {
    if (!requireLogin()) {
        return false;
    }

    const role = getUserRole();

    if (role !== "seller") {
        window.location.href = "buyer.html";
        return false;
    }

    return true;
}


// ------------------------------------------------------------
// CART
// ------------------------------------------------------------

function getCartKey() {
    const userId = getUserId();

    if (!userId) {
        return "kaviyaMartCart_guest";
    }

    return `kaviyaMartCart_user_${userId}`;
}


function getCart() {
    try {
        const cart = localStorage.getItem(getCartKey());

        if (!cart) {
            return [];
        }

        const parsed = JSON.parse(cart);

        return Array.isArray(parsed) ? parsed : [];

    } catch (error) {
        console.error("Cart read error:", error);
        return [];
    }
}


function saveCart(cart) {
    localStorage.setItem(
        getCartKey(),
        JSON.stringify(cart)
    );

    updateCartCount();
}


function updateCartCount() {
    const cartCountElements =
        document.querySelectorAll("#cartCount");

    const cart = getCart();

    const totalQuantity = cart.reduce(
        (total, item) =>
            total + Number(item.quantity || 0),
        0
    );

    cartCountElements.forEach(element => {
        element.textContent = totalQuantity;
    });
}


// ------------------------------------------------------------
// ADD TO CART
// ------------------------------------------------------------

function addToCart(product) {

    if (!isLoggedIn()) {
        showMessage(
            "Please login to add products to your cart.",
            "error"
        );

        setTimeout(() => {
            window.location.href = "login.html";
        }, 700);

        return;
    }

    if (getUserRole() !== "buyer") {
        showMessage(
            "Only buyers can add products to cart.",
            "error"
        );

        return;
    }

    const cart = getCart();

    const productId = Number(product.product_id);

    const existingItem = cart.find(
        item => Number(item.product_id) === productId
    );

    if (existingItem) {
        existingItem.quantity =
            Number(existingItem.quantity || 0) + 1;
    } else {
        cart.push({
            product_id: productId,
            product_name: product.product_name,
            category: product.category || "",
            price: Number(product.price || 0),
            description: product.description || "",
            image_path: product.image_path || "",
            rating: Number(product.rating || 0),
            quantity: 1
        });
    }

    saveCart(cart);

    showMessage("Product added to cart.");
}


// ------------------------------------------------------------
// REMOVE FROM CART
// ------------------------------------------------------------

function removeFromCart(productId) {

    const cart = getCart();

    const updatedCart = cart.filter(
        item =>
            Number(item.product_id) !== Number(productId)
    );

    saveCart(updatedCart);

    renderCart();
}


// ------------------------------------------------------------
// CHANGE CART QUANTITY
// ------------------------------------------------------------

function changeCartQuantity(productId, change) {

    const cart = getCart();

    const item = cart.find(
        product =>
            Number(product.product_id) === Number(productId)
    );

    if (!item) {
        return;
    }

    item.quantity =
        Number(item.quantity || 1) + Number(change);

    if (item.quantity <= 0) {
        removeFromCart(productId);
        return;
    }

    saveCart(cart);

    renderCart();
}


// ------------------------------------------------------------
// CART TOTAL
// ------------------------------------------------------------

function getCartTotal() {

    const cart = getCart();

    return cart.reduce(
        (total, item) =>
            total +
            Number(item.price || 0) *
            Number(item.quantity || 0),
        0
    );
}


// ------------------------------------------------------------
// FORMAT PRICE
// ------------------------------------------------------------

function formatPrice(price) {

    const value = Number(price || 0);

    return "₹" + value.toLocaleString("en-IN", {
        minimumFractionDigits: 2,
        maximumFractionDigits: 2
    });
}


// ------------------------------------------------------------
// RENDER CART
// ------------------------------------------------------------

function renderCart() {

    const cartItemsContainer =
        document.getElementById("cartItems");

    const cartTotalElement =
        document.getElementById("cartTotal");

    if (!cartItemsContainer) {
        return;
    }

    const cart = getCart();

    if (cart.length === 0) {

        cartItemsContainer.innerHTML = `
            <div class="empty-cart">
                <div style="font-size:42px;">🛒</div>
                <h3>Your cart is empty</h3>
                <p>Add some products to continue shopping.</p>
            </div>
        `;

        if (cartTotalElement) {
            cartTotalElement.textContent = "₹0.00";
        }

        return;
    }

    cartItemsContainer.innerHTML = "";

    cart.forEach(item => {

        const itemElement =
            document.createElement("div");

        itemElement.className = "cart-item";

        const imagePath =
            getImagePath(item.image_path);

        itemElement.innerHTML = `
            <div class="cart-item-image">
                <img
                    src="${escapeHtml(imagePath)}"
                    alt="${escapeHtml(item.product_name || "Product")}"
                    onerror="this.style.display='none';"
                >
            </div>

            <div class="cart-item-info">

                <h4>
                    ${escapeHtml(item.product_name || "Product")}
                </h4>

                <p>
                    ${formatPrice(item.price)}
                </p>

                <div class="cart-quantity">

                    <button
                        type="button"
                        onclick="changeCartQuantity(${Number(item.product_id)}, -1)"
                    >
                        −
                    </button>

                    <span>
                        ${Number(item.quantity || 1)}
                    </span>

                    <button
                        type="button"
                        onclick="changeCartQuantity(${Number(item.product_id)}, 1)"
                    >
                        +
                    </button>

                </div>

                <button
                    type="button"
                    class="remove-cart-item"
                    onclick="removeFromCart(${Number(item.product_id)})"
                >
                    Remove
                </button>

            </div>
        `;

        cartItemsContainer.appendChild(itemElement);
    });

    if (cartTotalElement) {
        cartTotalElement.textContent =
            formatPrice(getCartTotal());
    }
}


// ------------------------------------------------------------
// OPEN CART
// ------------------------------------------------------------

function openCart() {

    const cartPanel =
        document.getElementById("cartPanel");

    const cartOverlay =
        document.getElementById("cartOverlay");

    if (cartPanel) {
        cartPanel.classList.add("active");
    }

    if (cartOverlay) {
        cartOverlay.classList.add("active");
    }

    renderCart();
}


// ------------------------------------------------------------
// CLOSE CART
// ------------------------------------------------------------

function closeCart() {

    const cartPanel =
        document.getElementById("cartPanel");

    const cartOverlay =
        document.getElementById("cartOverlay");

    if (cartPanel) {
        cartPanel.classList.remove("active");
    }

    if (cartOverlay) {
        cartOverlay.classList.remove("active");
    }
}


// ------------------------------------------------------------
// BUY NOW
// ------------------------------------------------------------

function buyNow(product) {

    if (!isLoggedIn()) {

        showMessage(
            "Please login to buy this product.",
            "error"
        );

        setTimeout(() => {
            window.location.href = "login.html";
        }, 700);

        return;
    }

    if (getUserRole() !== "buyer") {

        showMessage(
            "Only buyers can purchase products.",
            "error"
        );

        return;
    }

    const checkoutProduct = {

        product_id:
            Number(product.product_id),

        product_name:
            product.product_name,

        category:
            product.category || "",

        price:
            Number(product.price || 0),

        description:
            product.description || "",

        image_path:
            product.image_path || "",

        rating:
            Number(product.rating || 0),

        quantity: 1
    };

    sessionStorage.setItem(
        "kaviyaMartBuyNow",
        JSON.stringify(checkoutProduct)
    );

    window.location.href =
        "checkout.html";
}


// ------------------------------------------------------------
// CHECKOUT FROM CART
// ------------------------------------------------------------

function goToCheckout() {

    if (!isLoggedIn()) {
        window.location.href = "login.html";
        return;
    }

    if (getUserRole() !== "buyer") {
        showMessage(
            "Only buyers can checkout.",
            "error"
        );
        return;
    }

    const cart = getCart();

    if (cart.length === 0) {
        showMessage(
            "Your cart is empty.",
            "error"
        );
        return;
    }

    // Remove Buy Now item so checkout uses cart
    sessionStorage.removeItem(
        "kaviyaMartBuyNow"
    );

    window.location.href =
        "checkout.html";
}


// ------------------------------------------------------------
// PRODUCT IMAGE PATH
// ------------------------------------------------------------

function getImagePath(imagePath) {

    if (!imagePath) {
        return "";
    }

    let path = String(imagePath).trim();

    if (!path) {
        return "";
    }

    // Already an absolute URL
    if (
        path.startsWith("http://") ||
        path.startsWith("https://") ||
        path.startsWith("data:")
    ) {
        return path;
    }

    // Windows path stored in database
    path = path.replace(/\\/g, "/");

    if (path.includes("/uploads/")) {
        return "/uploads/" +
            path.split("/uploads/")[1];
    }

    if (path.startsWith("uploads/")) {
        return "/" + path;
    }

    if (path.startsWith("/uploads/")) {
        return path;
    }

    // If only filename is stored
    if (
        !path.includes("/") &&
        !path.includes(":")
    ) {
        return "/uploads/" + path;
    }

    return path;
}


// ------------------------------------------------------------
// LOAD PRODUCTS
// ------------------------------------------------------------

async function loadProducts(category = "") {

    const productGrid =
        document.getElementById("productGrid");

    if (!productGrid) {
        return;
    }

    productGrid.innerHTML = `
        <div class="loading-products">
            <div>Loading products...</div>
        </div>
    `;

    try {

        const response =
            await fetch("/api/products");

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const result =
            await response.json();

        let products = [];

        if (Array.isArray(result)) {
            products = result;
        } else if (Array.isArray(result.products)) {
            products = result.products;
        } else if (Array.isArray(result.data)) {
            products = result.data;
        }

        if (category) {

            const selectedCategory =
                category.toLowerCase();

            products = products.filter(product =>
                String(product.category || "")
                    .toLowerCase() === selectedCategory
            );
        }

        displayProducts(products);

    } catch (error) {

        console.error(
            "Product loading error:",
            error
        );

        productGrid.innerHTML = `
            <div class="error-message">
                Unable to load products.
                Please make sure KaviyaMart server is running.
            </div>
        `;
    }
}


// ------------------------------------------------------------
// DISPLAY PRODUCTS
// ------------------------------------------------------------

function displayProducts(products) {

    const productGrid =
        document.getElementById("productGrid");

    if (!productGrid) {
        return;
    }

    if (!products || products.length === 0) {

        productGrid.innerHTML = `
            <div class="empty-products">
                <h3>No products found</h3>
                <p>Try another category or search.</p>
            </div>
        `;

        return;
    }

    productGrid.innerHTML = "";

    products.forEach(product => {

        const card =
            document.createElement("div");

        card.className = "product-card";

        const imagePath =
            getImagePath(product.image_path);

        const rating =
            Number(product.rating || 0);

        card.innerHTML = `

            <div class="product-image">

                ${
                    imagePath
                    ?
                    `
                    <img
                        src="${escapeHtml(imagePath)}"
                        alt="${escapeHtml(product.product_name || "Product")}"
                        onerror="this.style.display='none';"
                    >
                    `
                    :
                    `
                    <div class="no-product-image">
                        No Image
                    </div>
                    `
                }

            </div>

            <div class="product-info">

                <span class="product-category">
                    ${escapeHtml(product.category || "Product")}
                </span>

                <h3>
                    ${escapeHtml(product.product_name || "Unnamed Product")}
                </h3>

                <div class="product-rating">
                    ${createStars(rating)}
                    <span>${rating.toFixed(1)}</span>
                </div>

                <p class="product-description">
                    ${escapeHtml(
                        product.description || "No description available."
                    )}
                </p>

                <div class="product-bottom">

                    <span class="product-price">
                        ${formatPrice(product.price)}
                    </span>

                </div>

                <div class="product-actions">

                    <button
                        type="button"
                        class="btn add-cart-btn"
                        onclick='addToCart(${JSON.stringify(product).replace(/'/g, "&#39;")})'
                    >
                        Add to Cart
                    </button>

                    <button
                        type="button"
                        class="btn buy-now-btn"
                        onclick='buyNow(${JSON.stringify(product).replace(/'/g, "&#39;")})'
                    >
                        Buy Now
                    </button>

                </div>

            </div>
        `;

        productGrid.appendChild(card);
    });
}


// ------------------------------------------------------------
// SEARCH PRODUCTS
// ------------------------------------------------------------

function searchProducts() {

    const searchInput =
        document.getElementById("productSearch");

    const productCards =
        document.querySelectorAll(".product-card");

    if (!searchInput) {
        return;
    }

    const searchText =
        searchInput.value
            .trim()
            .toLowerCase();

    productCards.forEach(card => {

        const text =
            card.textContent.toLowerCase();

        if (text.includes(searchText)) {
            card.style.display = "";
        } else {
            card.style.display = "none";
        }
    });
}


// ------------------------------------------------------------
// CREATE RATING STARS
// ------------------------------------------------------------

function createStars(rating) {

    const value =
        Math.max(0, Math.min(5, Number(rating || 0)));

    const rounded =
        Math.round(value);

    let stars = "";

    for (let i = 1; i <= 5; i++) {
        stars += i <= rounded ? "★" : "☆";
    }

    return stars;
}


// ------------------------------------------------------------
// ADD PRODUCT - SELLER
// ------------------------------------------------------------

async function handleAddProduct(event) {

    event.preventDefault();

    if (!requireSeller()) {
        return;
    }

    const nameInput =
        document.getElementById("productName");

    const categoryInput =
        document.getElementById("productCategory");

    const priceInput =
        document.getElementById("productPrice");

    const descriptionInput =
        document.getElementById("productDescription");

    const imageInput =
        document.getElementById("productImage");

    if (
        !nameInput ||
        !categoryInput ||
        !priceInput ||
        !descriptionInput ||
        !imageInput
    ) {
        return;
    }

    const productName =
        nameInput.value.trim();

    const category =
        categoryInput.value;

    const price =
        Number(priceInput.value);

    const description =
        descriptionInput.value.trim();

    const imageFile =
        imageInput.files[0];

    if (!productName || !category || !description) {

        showMessage(
            "Please fill in all product details.",
            "error"
        );

        return;
    }

    if (!Number.isFinite(price) || price < 0) {

        showMessage(
            "Please enter a valid price.",
            "error"
        );

        return;
    }

    if (!imageFile) {

        showMessage(
            "Please select a product image.",
            "error"
        );

        return;
    }

    const allowedTypes = [
        "image/jpeg",
        "image/png",
        "image/webp"
    ];

    if (!allowedTypes.includes(imageFile.type)) {

        showMessage(
            "Only JPG, PNG and WEBP images are allowed.",
            "error"
        );

        return;
    }

    const formData =
        new FormData();

    formData.append(
        "seller_id",
        getUserId()
    );

    formData.append(
        "product_name",
        productName
    );

    formData.append(
        "category",
        category
    );

    formData.append(
        "price",
        price
    );

    formData.append(
        "description",
        description
    );

    formData.append(
        "image",
        imageFile
    );

    try {

        const response =
            await fetch("/api/products", {
                method: "POST",
                body: formData
            });

        const result =
            await response.json();

        if (!response.ok) {

            showMessage(
                result.message ||
                result.error ||
                "Failed to add product.",
                "error"
            );

            return;
        }

        showMessage(
            "Product added successfully!"
        );

        setTimeout(() => {
            window.location.href =
                "seller.html";
        }, 800);

    } catch (error) {

        console.error(
            "Add product error:",
            error
        );

        showMessage(
            "Unable to connect to server.",
            "error"
        );
    }
}


// ------------------------------------------------------------
// SELLER PRODUCTS
// ------------------------------------------------------------

async function loadSellerProducts() {

    const container =
        document.getElementById("sellerProducts");

    if (!container) {
        return;
    }

    if (!requireSeller()) {
        return;
    }

    const sellerId =
        getUserId();

    container.innerHTML = `
        <div class="loading-products">
            Loading your products...
        </div>
    `;

    try {

        const response =
            await fetch(
                `/api/products/seller/${sellerId}`
            );

        if (!response.ok) {
            throw new Error(
                `HTTP ${response.status}`
            );
        }

        const result =
            await response.json();

        let products = [];

        if (Array.isArray(result)) {
            products = result;
        } else if (Array.isArray(result.products)) {
            products = result.products;
        } else if (Array.isArray(result.data)) {
            products = result.data;
        }

        displaySellerProducts(products);

        updateSellerStats(products);

    } catch (error) {

        console.error(
            "Seller products error:",
            error
        );

        container.innerHTML = `
            <div class="error-message">
                Unable to load your products.
            </div>
        `;
    }
}


// ------------------------------------------------------------
// DISPLAY SELLER PRODUCTS
// ------------------------------------------------------------

function displaySellerProducts(products) {

    const container =
        document.getElementById("sellerProducts");

    if (!container) {
        return;
    }

    if (!products || products.length === 0) {

        container.innerHTML = `
            <div class="empty-products">
                <h3>No products yet</h3>
                <p>Add your first product to start selling.</p>

                <a
                    href="add-product.html"
                    class="btn"
                >
                    Add Product
                </a>
            </div>
        `;

        return;
    }

    container.innerHTML = "";

    products.forEach(product => {

        const card =
            document.createElement("div");

        card.className = "seller-product-card";

        const imagePath =
            getImagePath(product.image_path);

        card.innerHTML = `

            <div class="seller-product-image">

                ${
                    imagePath
                    ?
                    `
                    <img
                        src="${escapeHtml(imagePath)}"
                        alt="${escapeHtml(product.product_name || "Product")}"
                        onerror="this.style.display='none';"
                    >
                    `
                    :
                    `
                    <div class="no-product-image">
                        No Image
                    </div>
                    `
                }

            </div>

            <div class="seller-product-info">

                <span class="product-category">
                    ${escapeHtml(product.category || "")}
                </span>

                <h3>
                    ${escapeHtml(product.product_name || "Unnamed Product")}
                </h3>

                <p>
                    ${formatPrice(product.price)}
                </p>

                <p>
                    ${escapeHtml(
                        product.description || ""
                    )}
                </p>

                <div class="seller-product-actions">

                    <button
                        type="button"
                        class="btn"
                        onclick="editProduct(${Number(product.product_id)})"
                    >
                        Edit
                    </button>

                    <button
                        type="button"
                        class="btn delete-product-btn"
                        onclick="deleteProduct(${Number(product.product_id)})"
                    >
                        Delete
                    </button>

                </div>

            </div>
        `;

        container.appendChild(card);
    });
}


// ------------------------------------------------------------
// SELLER STATS
// ------------------------------------------------------------

function updateSellerStats(products) {

    const productCount =
        document.getElementById("productCount");

    if (productCount) {
        productCount.textContent =
            products.length;
    }

    const orderCount =
        document.getElementById("orderCount");

    if (orderCount) {
        orderCount.textContent = "0";
    }

    const salesAmount =
        document.getElementById("salesAmount");

    if (salesAmount) {
        salesAmount.textContent = "₹0";
    }

    const sellerName =
        document.getElementById("sellerName");

    if (sellerName) {
        sellerName.textContent =
            getUserName();
    }
}


// ------------------------------------------------------------
// EDIT PRODUCT
// ------------------------------------------------------------

function editProduct(productId) {

    if (!requireSeller()) {
        return;
    }

    window.location.href =
        `edit-product.html?id=${Number(productId)}`;
}


// ------------------------------------------------------------
// LOAD SINGLE PRODUCT FOR EDIT
// ------------------------------------------------------------

async function loadEditProduct() {

    const form =
        document.getElementById("editProductForm");

    if (!form) {
        return;
    }

    if (!requireSeller()) {
        return;
    }

    const params =
        new URLSearchParams(window.location.search);

    const productId =
        params.get("id");

    if (!productId) {

        showEditMessage(
            "Product ID is missing.",
            true
        );

        return;
    }

    try {

        const response =
            await fetch(
                `/api/products/${Number(productId)}`
            );

        const result =
            await response.json();

        if (!response.ok) {

            showEditMessage(
                result.message ||
                result.error ||
                "Unable to load product.",
                true
            );

            return;
        }

        const product =
            result.product || result;

        fillEditProductForm(product);

    } catch (error) {

        console.error(
            "Load edit product error:",
            error
        );

        showEditMessage(
            "Unable to connect to server.",
            true
        );
    }
}


// ------------------------------------------------------------
// FILL EDIT FORM
// ------------------------------------------------------------

function fillEditProductForm(product) {

    const nameInput =
        document.getElementById("editProductName");

    const categoryInput =
        document.getElementById("editProductCategory");

    const priceInput =
        document.getElementById("editProductPrice");

    const descriptionInput =
        document.getElementById("editProductDescription");

    const currentImageContainer =
        document.getElementById("currentImageContainer");

    const productIdInput =
        document.getElementById("editProductId");

    if (nameInput) {
        nameInput.value =
            product.product_name || "";
    }

    if (categoryInput) {
        categoryInput.value =
            product.category || "";
    }

    if (priceInput) {
        priceInput.value =
            product.price || "";
    }

    if (descriptionInput) {
        descriptionInput.value =
            product.description || "";
    }

    if (productIdInput) {
        productIdInput.value =
            product.product_id || "";
    }

    if (currentImageContainer) {

        const imagePath =
            getImagePath(product.image_path);

        if (imagePath) {

            currentImageContainer.innerHTML = `
                <p><strong>Current Image</strong></p>

                <img
                    src="${escapeHtml(imagePath)}"
                    alt="Current product image"
                    style="
                        max-width:220px;
                        max-height:220px;
                        object-fit:contain;
                        border-radius:12px;
                        margin-top:10px;
                    "
                    onerror="this.style.display='none';"
                >
            `;

        } else {

            currentImageContainer.innerHTML = `
                <p>No current image.</p>
            `;
        }
    }
}


// ------------------------------------------------------------
// UPDATE PRODUCT
// ------------------------------------------------------------

async function handleEditProduct(event) {

    event.preventDefault();

    if (!requireSeller()) {
        return;
    }

    const params =
        new URLSearchParams(window.location.search);

    const productId =
        params.get("id");

    if (!productId) {
        showEditMessage(
            "Product ID is missing.",
            true
        );
        return;
    }

    const nameInput =
        document.getElementById("editProductName");

    const categoryInput =
        document.getElementById("editProductCategory");

    const priceInput =
        document.getElementById("editProductPrice");

    const descriptionInput =
        document.getElementById("editProductDescription");

    const imageInput =
        document.getElementById("editProductImage");

    if (
        !nameInput ||
        !categoryInput ||
        !priceInput ||
        !descriptionInput
    ) {
        return;
    }

    const productName =
        nameInput.value.trim();

    const category =
        categoryInput.value;

    const price =
        Number(priceInput.value);

    const description =
        descriptionInput.value.trim();

    if (!productName || !category || !description) {

        showEditMessage(
            "Please fill in all product details.",
            true
        );

        return;
    }

    if (!Number.isFinite(price) || price < 0) {

        showEditMessage(
            "Please enter a valid price.",
            true
        );

        return;
    }

    const formData =
        new FormData();

    formData.append(
        "seller_id",
        getUserId()
    );

    formData.append(
        "product_name",
        productName
    );

    formData.append(
        "category",
        category
    );

    formData.append(
        "price",
        price
    );

    formData.append(
        "description",
        description
    );

    if (
        imageInput &&
        imageInput.files &&
        imageInput.files.length > 0
    ) {

        const imageFile =
            imageInput.files[0];

        const allowedTypes = [
            "image/jpeg",
            "image/png",
            "image/webp"
        ];

        if (!allowedTypes.includes(imageFile.type)) {

            showEditMessage(
                "Only JPG, PNG and WEBP images are allowed.",
                true
            );

            return;
        }

        formData.append(
            "image",
            imageFile
        );
    }

    const saveButton =
        document.getElementById("saveProductBtn");

    if (saveButton) {
        saveButton.disabled = true;
        saveButton.textContent =
            "Saving...";
    }

    try {

        const response =
            await fetch(
                `/api/products/${Number(productId)}`,
                {
                    method: "PUT",
                    body: formData
                }
            );

        const result =
            await response.json();

        if (!response.ok) {

            showEditMessage(
                result.message ||
                result.error ||
                "Failed to update product.",
                true
            );

            return;
        }

        showEditMessage(
            "Product updated successfully!",
            false
        );

        setTimeout(() => {
            window.location.href =
                "seller.html";
        }, 1000);

    } catch (error) {

        console.error(
            "Update product error:",
            error
        );

        showEditMessage(
            "Unable to connect to server.",
            true
        );

    } finally {

        if (saveButton) {
            saveButton.disabled = false;
            saveButton.textContent =
                "Save Changes";
        }
    }
}


// ------------------------------------------------------------
// EDIT PRODUCT MESSAGE
// ------------------------------------------------------------

function showEditMessage(message, isError = false) {

    const element =
        document.getElementById("editProductMessage");

    if (!element) {
        showMessage(
            message,
            isError ? "error" : "success"
        );
        return;
    }

    element.textContent = message;

    element.className =
        isError
        ? "error-message"
        : "success-message";

    element.style.display = "block";
}


// ------------------------------------------------------------
// DELETE PRODUCT
// ------------------------------------------------------------

async function deleteProduct(productId) {

    if (!requireSeller()) {
        return;
    }

    const confirmed =
        window.confirm(
            "Are you sure you want to delete this product?"
        );

    if (!confirmed) {
        return;
    }

    try {

        const response =
            await fetch(
                `/api/products/${Number(productId)}`,
                {
                    method: "DELETE",
                    headers: {
                        "Content-Type": "application/json"
                    },
                    body: JSON.stringify({
                        seller_id: Number(getUserId())
                    })
                }
            );

        const result =
            await response.json();

        if (!response.ok) {

            showMessage(
                result.message ||
                result.error ||
                "Unable to delete product.",
                "error"
            );

            return;
        }

        showMessage(
            "Product deleted successfully."
        );

        loadSellerProducts();

    } catch (error) {

        console.error(
            "Delete product error:",
            error
        );

        showMessage(
            "Unable to connect to server.",
            "error"
        );
    }
}


// ------------------------------------------------------------
// IMAGE PREVIEW - ADD PRODUCT
// ------------------------------------------------------------

function setupImagePreview() {

    const imageInput =
        document.getElementById("productImage");

    const previewContainer =
        document.getElementById("imagePreview");

    const previewImage =
        document.getElementById("previewImage");

    if (
        !imageInput ||
        !previewContainer ||
        !previewImage
    ) {
        return;
    }

    imageInput.addEventListener(
        "change",
        function () {

            const file =
                this.files[0];

            if (!file) {

                previewContainer.style.display =
                    "none";

                previewImage.src = "";

                return;
            }

            if (!file.type.startsWith("image/")) {

                previewContainer.style.display =
                    "none";

                showMessage(
                    "Please select a valid image.",
                    "error"
                );

                return;
            }

            const reader =
                new FileReader();

            reader.onload = function (event) {

                previewImage.src =
                    event.target.result;

                previewContainer.style.display =
                    "block";
            };

            reader.readAsDataURL(file);
        }
    );
}


// ------------------------------------------------------------
// IMAGE PREVIEW - EDIT PRODUCT
// ------------------------------------------------------------

function setupEditImagePreview() {

    const imageInput =
        document.getElementById("editProductImage");

    const previewContainer =
        document.getElementById("newImagePreview");

    const previewImage =
        document.getElementById("editPreviewImage");

    if (
        !imageInput ||
        !previewContainer ||
        !previewImage
    ) {
        return;
    }

    imageInput.addEventListener(
        "change",
        function () {

            const file =
                this.files[0];

            if (!file) {

                previewContainer.style.display =
                    "none";

                previewImage.src = "";

                return;
            }

            const allowedTypes = [
                "image/jpeg",
                "image/png",
                "image/webp"
            ];

            if (!allowedTypes.includes(file.type)) {

                previewContainer.style.display =
                    "none";

                showEditMessage(
                    "Only JPG, PNG and WEBP images are allowed.",
                    true
                );

                this.value = "";

                return;
            }

            const reader =
                new FileReader();

            reader.onload = function (event) {

                previewImage.src =
                    event.target.result;

                previewContainer.style.display =
                    "block";
            };

            reader.readAsDataURL(file);
        }
    );
}


// ------------------------------------------------------------
// CHECKOUT PAGE HELPERS
// ------------------------------------------------------------

function getCheckoutItems() {

    const buyNowData =
        sessionStorage.getItem(
            "kaviyaMartBuyNow"
        );

    if (buyNowData) {

        try {

            const product =
                JSON.parse(buyNowData);

            if (product) {
                return [product];
            }

        } catch (error) {

            console.error(
                "Buy Now data error:",
                error
            );
        }
    }

    return getCart();
}


// ------------------------------------------------------------
// CLEAR CHECKOUT DATA
// ------------------------------------------------------------

function clearCheckoutData() {

    const wasBuyNow =
        !!sessionStorage.getItem(
            "kaviyaMartBuyNow"
        );

    sessionStorage.removeItem(
        "kaviyaMartBuyNow"
    );

    if (!wasBuyNow) {
        saveCart([]);
    }
}


// ------------------------------------------------------------
// CHECKOUT TOTAL
// ------------------------------------------------------------

function getCheckoutTotal(items) {

    if (!Array.isArray(items)) {
        return 0;
    }

    return items.reduce(
        (total, item) =>
            total +
            Number(item.price || 0) *
            Number(item.quantity || 1),
        0
    );
}


// ------------------------------------------------------------
// HTML ESCAPE
// ------------------------------------------------------------

function escapeHtml(value) {

    if (value === null || value === undefined) {
        return "";
    }

    return String(value)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}


// ------------------------------------------------------------
// UPDATE NAVIGATION
// ------------------------------------------------------------

function updateNavigation() {

    const loggedIn =
        isLoggedIn();

    const role =
        getUserRole();

    const loginLinks =
        document.querySelectorAll(
            ".login-link"
        );

    loginLinks.forEach(link => {
        link.style.display =
            loggedIn ? "none" : "";
    });

    const logoutLinks =
        document.querySelectorAll(
            "#logoutLink"
        );

    logoutLinks.forEach(link => {

        if (loggedIn) {
            link.style.display = "";
        } else {
            link.style.display = "none";
        }
    });

    const buyerLinks =
        document.querySelectorAll(
            ".buyer-only"
        );

    buyerLinks.forEach(link => {
        link.style.display =
            role === "buyer" ? "" : "none";
    });

    const sellerLinks =
        document.querySelectorAll(
            ".seller-only"
        );

    sellerLinks.forEach(link => {
        link.style.display =
            role === "seller" ? "" : "none";
    });
}


// ------------------------------------------------------------
// PROTECT PAGES
// ------------------------------------------------------------

function protectCurrentPage() {

    const page =
        window.location.pathname
            .split("/")
            .pop()
            .toLowerCase();

    if (
        page === "buyer.html" ||
        page === "checkout.html"
    ) {
        requireBuyer();
    }

    if (
        page === "seller.html" ||
        page === "add-product.html" ||
        page === "edit-product.html"
    ) {
        requireSeller();
    }
}


// ------------------------------------------------------------
// DOM READY
// ------------------------------------------------------------

document.addEventListener(
    "DOMContentLoaded",
    function () {

        // Login form
        const loginForm =
            document.getElementById("loginForm");

        if (loginForm) {
            loginForm.addEventListener(
                "submit",
                handleLogin
            );
        }


        // Register form
        const registerForm =
            document.getElementById("registerForm");

        if (registerForm) {
            registerForm.addEventListener(
                "submit",
                handleRegister
            );
        }


        // Add product form
        const productForm =
            document.getElementById("productForm");

        if (productForm) {
            productForm.addEventListener(
                "submit",
                handleAddProduct
            );
        }


        // Edit product form
        const editProductForm =
            document.getElementById("editProductForm");

        if (editProductForm) {
            editProductForm.addEventListener(
                "submit",
                handleEditProduct
            );
        }


        // Image previews
        setupImagePreview();
        setupEditImagePreview();


        // Logout
        const logoutLink =
            document.getElementById("logoutLink");

        if (logoutLink) {

            logoutLink.addEventListener(
                "click",
                function (event) {

                    event.preventDefault();

                    logout();
                }
            );
        }


        // Cart link
        const cartLink =
            document.getElementById("cartLink");

        if (cartLink) {

            cartLink.addEventListener(
                "click",
                function (event) {

                    event.preventDefault();

                    if (!isLoggedIn()) {

                        window.location.href =
                            "login.html";

                        return;
                    }

                    openCart();
                }
            );
        }


        // Close cart
        const closeCartButton =
            document.getElementById("closeCart");

        if (closeCartButton) {

            closeCartButton.addEventListener(
                "click",
                closeCart
            );
        }


        // Cart overlay
        const cartOverlay =
            document.getElementById("cartOverlay");

        if (cartOverlay) {

            cartOverlay.addEventListener(
                "click",
                closeCart
            );
        }


        // Checkout button
        const checkoutButton =
            document.getElementById("checkoutBtn");

        if (checkoutButton) {

            checkoutButton.addEventListener(
                "click",
                goToCheckout
            );
        }


        // Product search
        const searchInput =
            document.getElementById("productSearch");

        if (searchInput) {

            searchInput.addEventListener(
                "input",
                searchProducts
            );
        }


        // Load products on buyer page
        if (
            document.getElementById("productGrid")
        ) {
            loadProducts();
        }


        // Load seller products
        if (
            document.getElementById("sellerProducts")
        ) {
            loadSellerProducts();
        }


        // Load edit product
        if (
            document.getElementById("editProductForm")
        ) {
            loadEditProduct();
        }


        // Update cart count
        updateCartCount();


        // Navigation
        updateNavigation();


        // Page protection
        protectCurrentPage();
    }
);