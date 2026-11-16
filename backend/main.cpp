#include "storage/file_storage.hpp"
#include "crow.h"
#include "crow/middlewares/cors.h"
#include "inventory_engine.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <mutex>
#include <exception>
#include <cmath>
#include <climits>
using namespace std;
using namespace crow;

int main() {
    crow::App<crow::CORSHandler> app;
    
crow::App<crow::CORSHandler> app;

auto& cors = app.get_middleware<crow::CORSHandler>();

cors.prefix("/api/")
    .origin("https://ahtishamasghar.github.io")
    .methods("GET"_method, "POST"_method, "PUT"_method,
             "DELETE"_method, "OPTIONS"_method)
    .headers("Content-Type");
    InventoryEngine inventory;
    FileStorage storage("storage/products.json");
    mutex inventoryMutex;

    try {
        vector<Product> savedProducts = storage.load();

        for (const Product& product : savedProducts) {
            if (!inventory.addProduct(product)) {
                throw runtime_error("Invalid or duplicate stored product");
            }
        }

        cout << "Loaded " << savedProducts.size()
            << " products from storage." << endl;
    }
    catch (const exception& e) {
        cerr << "Storage error: " << e.what() << endl;
        return 1;
    }
    CROW_ROUTE(app, "/")([]() {
    return crow::response(200, "Inventory API is running");
    });

    CROW_ROUTE(app, "/api/health")([]() {
        json::wvalue response;
        response["status"] = "ok";
        response["service"] = "Inventory Optimization Engine";
        return response;
    });

    CROW_ROUTE(app, "/api/products")
    ([&inventory, &inventoryMutex]() {
        lock_guard<mutex> lock(inventoryMutex);
        vector<Product> products = inventory.getAllProducts();
        json::wvalue response;
        response["products"] = json::wvalue::list();

        for (int i = 0; i < (int)products.size(); i++) {
            response["products"][i]["id"] = products[i].id;
            response["products"][i]["name"] = products[i].name;
            response["products"][i]["category"] = products[i].category;
            response["products"][i]["price"] = products[i].price;
            response["products"][i]["quantity"] = products[i].quantity;
            response["products"][i]["minStock"] = products[i].minStock;
        }

        return response;
    });


CROW_ROUTE(app, "/api/products").methods(HTTPMethod::POST)
([&inventory, &storage, &inventoryMutex](const request& req) {
    auto body = json::load(req.body);
    if (!body) {
        return response(400, "Invalid JSON");
    }
    if (!body.has("id") || !body.has("name") ||
        !body.has("category") || !body.has("price") ||
        !body.has("quantity") || !body.has("minStock")) {
        return response(400, "Missing product fields");
    }
    try {
        if (body["id"].t() != json::type::Number ||
            body["name"].t() != json::type::String ||
            body["category"].t() != json::type::String ||
            body["price"].t() != json::type::Number ||
            body["quantity"].t() != json::type::Number ||
            body["minStock"].t() != json::type::Number) {
            return response(400, "Invalid product field types");
        }
        double id = body["id"].d();
        double quantity = body["quantity"].d();
        double minStock = body["minStock"].d();
        if (!isfinite(id) || !isfinite(quantity) ||
            !isfinite(minStock) ||
            floor(id) != id ||
            floor(quantity) != quantity ||
            floor(minStock) != minStock ||
            id < 0 || id > INT_MAX ||
            quantity < 0 || quantity > INT_MAX ||
            minStock < 0 || minStock > INT_MAX) {
            return response(400, "Invalid integer values");
        }
        Product product;
        product.id = static_cast<int>(id);
        product.name = body["name"].s();
        product.category = body["category"].s();
        product.price = body["price"].d();
        product.quantity = static_cast<int>(quantity);
        product.minStock = static_cast<int>(minStock);
        lock_guard<mutex> lock(inventoryMutex);
        if (!inventory.addProduct(product)) {
            return response(400, "Invalid or duplicate product");
        }
        if (!storage.save(inventory.getAllProducts())) {
            inventory.removeProduct(product.id);
            return response(500, "Failed to save product");
        }
        return response(201, "Product added successfully");
    }
    catch (const std::exception&) {
        return response(400, "Invalid product field types");
    }
});

    CROW_ROUTE(app, "/api/products/lowest-stock")
    ([&inventory, &inventoryMutex]() {
        lock_guard<mutex> lock(inventoryMutex);
        const Product* product = inventory.getLowestStock();

        if (product == nullptr) {
            return response(404, "Inventory is empty");
        }

        json::wvalue result;

        result["id"] = product->id;
        result["name"] = product->name;
        result["category"] = product->category;
        result["price"] = product->price;
        result["quantity"] = product->quantity;
        result["minStock"] = product->minStock;

        return response(200, result);
    });

    CROW_ROUTE(app, "/api/products/<int>")
    ([&inventory, &inventoryMutex](int id) {
        lock_guard<mutex> lock(inventoryMutex);
        const Product* product = inventory.getProduct(id);

        if (product == nullptr) {
            return response(404, "Product not found");
        }

        json::wvalue result;

        result["id"] = product->id;
        result["name"] = product->name;
        result["category"] = product->category;
        result["price"] = product->price;
        result["quantity"] = product->quantity;
        result["minStock"] = product->minStock;

        return response(200, result);
    });


    CROW_ROUTE(app, "/api/products/<int>").methods(HTTPMethod::PUT)
    ([&inventory, &storage, &inventoryMutex](const request& req, int id) {
        auto body = json::load(req.body);

        if (!body) {
            return response(400, "Invalid JSON");
        }

        if (!body.has("name") || !body.has("category") ||
            !body.has("price") || !body.has("quantity") ||
            !body.has("minStock")) {
            return response(400, "Missing product fields");
        }

        try {
            if (body["name"].t() != json::type::String ||
                body["category"].t() != json::type::String ||
                body["price"].t() != json::type::Number ||
                body["quantity"].t() != json::type::Number ||
                body["minStock"].t() != json::type::Number) {
                return response(400, "Invalid product field types");
            }

            double quantity = body["quantity"].d();
            double minStock = body["minStock"].d();

            if (!isfinite(quantity) || !isfinite(minStock) ||
                floor(quantity) != quantity ||
                floor(minStock) != minStock ||
                quantity < 0 || quantity > INT_MAX ||
                minStock < 0 || minStock > INT_MAX) {
                return response(400, "Invalid integer values");
            }
            lock_guard<mutex> lock(inventoryMutex);

            const Product* existing = inventory.getProduct(id);

            if (existing == nullptr) {
                return response(404, "Product not found");
            }

            Product oldProduct = *existing;

            Product product;
            product.id = id;
            product.name = body["name"].s();
            product.category = body["category"].s();
            product.price = body["price"].d();
            product.quantity = static_cast<int>(quantity);
            product.minStock = static_cast<int>(minStock);

            if (!inventory.updateProduct(product)) {
                return response(400, "Invalid product data");
            }

            if (!storage.save(inventory.getAllProducts())) {
                inventory.updateProduct(oldProduct);
                return response(500, "Failed to save updated product");
            }

            return response(200, "Product updated successfully");
        }
        catch (const std::exception&) {
            return response(400, "Invalid product field types");
        }
    });



    CROW_ROUTE(app, "/api/products/<int>").methods(HTTPMethod::DELETE)
    ([&inventory, &storage, &inventoryMutex](int id) {
        lock_guard<mutex> lock(inventoryMutex);

        const Product* product = inventory.getProduct(id);

        if (product == nullptr) {
            return response(404, "Product not found");
        }

        Product oldProduct = *product;

        if (!inventory.removeProduct(id)) {
            return response(500, "Failed to remove product");
        }

        if (!storage.save(inventory.getAllProducts())) {
            inventory.addProduct(oldProduct);
            return response(500, "Failed to save inventory");
        }

        return response(200, "Product deleted successfully");
    });

    CROW_ROUTE(app, "/api/products/search")
    ([&inventory, &inventoryMutex](const request& req) {
        const char* name = req.url_params.get("name");
        if (name == nullptr || string(name).find_first_not_of(" \t\r\n") == string::npos) {
            return response(400, "Please provide a product name.");
        }
        lock_guard<mutex> lock(inventoryMutex);
        vector<Product> products = inventory.searchProductsByName(name);

        json::wvalue result;
        result["products"] = json::wvalue::list();

        for (int i = 0; i < (int)products.size(); i++) {
            result["products"][i]["id"] = products[i].id;
            result["products"][i]["name"] = products[i].name;
            result["products"][i]["category"] = products[i].category;
            result["products"][i]["price"] = products[i].price;
            result["products"][i]["quantity"] = products[i].quantity;
            result["products"][i]["minStock"] = products[i].minStock;
        }

        return response(200, result);
    });

    CROW_ROUTE(app, "/api/products/sorted-by-price")
    ([&inventory, &inventoryMutex]() {
        lock_guard<mutex> lock(inventoryMutex);
        vector<Product> products = inventory.getProductsSortedByPrice();

        json::wvalue result;
        result["products"] = json::wvalue::list();

        for (int i = 0; i < (int)products.size(); i++) {
            result["products"][i]["id"] = products[i].id;
            result["products"][i]["name"] = products[i].name;
            result["products"][i]["category"] = products[i].category;
            result["products"][i]["price"] = products[i].price;
            result["products"][i]["quantity"] = products[i].quantity;
            result["products"][i]["minStock"] = products[i].minStock;
        }

        return result;
    });

    CROW_ROUTE(app, "/api/products/sorted-by-id")
    ([&inventory, &inventoryMutex]() {
        lock_guard<mutex> lock(inventoryMutex);
        vector<Product> products = inventory.getProductsSortedById();

        json::wvalue result;
        result["products"] = json::wvalue::list();

        for (int i = 0; i < (int)products.size(); i++) {
            result["products"][i]["id"] = products[i].id;
            result["products"][i]["name"] = products[i].name;
            result["products"][i]["category"] = products[i].category;
            result["products"][i]["price"] = products[i].price;
            result["products"][i]["quantity"] = products[i].quantity;
            result["products"][i]["minStock"] = products[i].minStock;
        }

        return result;
    });

    
    const char* portEnv = getenv("PORT");
    int port = portEnv ? stoi(portEnv) : 8080;
    
    app.bindaddr("0.0.0.0").port(port).run();
    return 0;
}
