// =====================================================
// REGISTER
// =====================================================

const registerForm =
    document.getElementById("registerForm");

if (registerForm) {

    registerForm.addEventListener(
        "submit",
        async function (event) {

            event.preventDefault();

            const data = {

                name:
                    document
                        .getElementById("registerName")
                        .value
                        .trim(),

                email:
                    document
                        .getElementById("registerEmail")
                        .value
                        .trim(),

                mobile:
                    document
                        .getElementById("registerMobile")
                        .value
                        .trim(),

                password:
                    document
                        .getElementById("registerPassword")
                        .value,

                role:
                    document
                        .getElementById("registerRole")
                        .value
            };


            try {

                const response =
                    await fetch(
                        "/api/register",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify(data)
                        }
                    );


                const result =
                    await response.json();


                if (result.success) {

                    alert(
                        "Registration successful! Please login."
                    );

                    window.location.href =
                        "login.html";

                }
                else {

                    alert(
                        result.message ||
                        "Registration failed."
                    );

                }

            }
            catch (error) {

                console.error(
                    "Register error:",
                    error
                );

                alert(
                    "Cannot connect to the server."
                );

            }

        }
    );

}



// =====================================================
// LOGIN
// =====================================================

const loginForm =
    document.getElementById("loginForm");

if (loginForm) {

    loginForm.addEventListener(
        "submit",
        async function (event) {

            event.preventDefault();


            const data = {

                email:
                    document
                        .getElementById("loginEmail")
                        .value
                        .trim(),

                password:
                    document
                        .getElementById("loginPassword")
                        .value

            };


            try {

                const response =
                    await fetch(
                        "/api/login",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify(data)
                        }
                    );


                const result =
                    await response.json();


                console.log(
                    "Login response:",
                    result
                );


                if (result.success) {

                    // Save logged-in user

                    localStorage.setItem(
                        "user_id",
                        result.user_id
                    );

                    localStorage.setItem(
                        "user_name",
                        result.name
                    );

                    localStorage.setItem(
                        "user_role",
                        result.role
                    );


                    // Redirect based on role

                    if (result.role === "buyer") {

                        window.location.href =
                            "buyer.html";

                    }
                    else if (
                        result.role === "seller"
                    ) {

                        window.location.href =
                            "seller.html";

                    }
                    else {

                        alert(
                            "Unknown user role."
                        );

                    }

                }
                else {

                    alert(
                        result.message ||
                        "Login failed."
                    );

                }

            }
            catch (error) {

                console.error(
                    "Login error:",
                    error
                );

                alert(
                    "Cannot connect to the server."
                );

            }

        }
    );

}



// =====================================================
// SELLER NAME
// =====================================================

const sellerName =
    document.getElementById("sellerName");

if (sellerName) {

    const name =
        localStorage.getItem("user_name");

    if (name) {

        sellerName.textContent =
            name;

    }

}



// =====================================================
// LOGOUT
// =====================================================

const logoutLink =
    document.getElementById("logoutLink");

if (logoutLink) {

    logoutLink.addEventListener(
        "click",
        function () {

            localStorage.removeItem(
                "user_id"
            );

            localStorage.removeItem(
                "user_name"
            );

            localStorage.removeItem(
                "user_role"
            );

        }
    );

}
// =====================================================
// ADD PRODUCT
// =====================================================

const productForm = document.getElementById("productForm");

if (productForm) {

    const imageInput =
        document.getElementById("productImage");

    const imagePreview =
        document.getElementById("imagePreview");

    const previewImage =
        document.getElementById("previewImage");


    // ---------------------------------------------
    // IMAGE PREVIEW
    // ---------------------------------------------

    if (imageInput) {

        imageInput.addEventListener("change", function () {

            const file =
                imageInput.files[0];

            if (!file) {

                imagePreview.style.display = "none";

                previewImage.src = "";

                return;
            }


            const allowedTypes = [
                "image/jpeg",
                "image/png",
                "image/webp"
            ];


            if (!allowedTypes.includes(file.type)) {

                alert(
                    "Please select a JPG, PNG or WEBP image."
                );

                imageInput.value = "";

                imagePreview.style.display = "none";

                return;
            }


            const reader =
                new FileReader();


            reader.onload = function (event) {

                previewImage.src =
                    event.target.result;

                imagePreview.style.display =
                    "block";
            };


            reader.readAsDataURL(file);

        });

    }


    // ---------------------------------------------
    // SUBMIT PRODUCT
    // ---------------------------------------------

    productForm.addEventListener(
        "submit",
        async function (event) {

            event.preventDefault();


            const sellerId =
                localStorage.getItem("user_id");

            const role =
                localStorage.getItem("user_role");


            if (!sellerId || role !== "seller") {

                alert(
                    "Please login as a seller."
                );

                window.location.href =
                    "login.html";

                return;
            }


            const imageFile =
                document.getElementById(
                    "productImage"
                ).files[0];


            if (!imageFile) {

                alert(
                    "Please choose a product image."
                );

                return;
            }


            const formData =
                new FormData();


            formData.append(
                "seller_id",
                sellerId
            );


            formData.append(
                "product_name",
                document.getElementById(
                    "productName"
                ).value.trim()
            );


            formData.append(
                "category",
                document.getElementById(
                    "productCategory"
                ).value
            );


            formData.append(
                "price",
                document.getElementById(
                    "productPrice"
                ).value
            );


            formData.append(
                "description",
                document.getElementById(
                    "productDescription"
                ).value.trim()
            );


            formData.append(
                "image",
                imageFile
            );


            try {

                const response =
                    await fetch(
                        "/api/products",
                        {
                            method: "POST",

                            body: formData
                        }
                    );


                const result =
                    await response.json();


                console.log(
                    "Add product response:",
                    result
                );


                if (response.ok &&
                    result.success) {

                    alert(
                        "Product added successfully!"
                    );


                    productForm.reset();


                    if (imagePreview) {

                        imagePreview.style.display =
                            "none";
                    }


                    window.location.href =
                        "seller.html";

                }
                else {

                    alert(
                        result.message ||
                        "Failed to add product."
                    );

                }

            }
            catch (error) {

                console.error(
                    "Add product error:",
                    error
                );


                alert(
                    "Cannot connect to the server."
                );

            }

        }
    );

}