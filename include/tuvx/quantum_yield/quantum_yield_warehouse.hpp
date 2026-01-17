#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <tuvx/quantum_yield/quantum_yield.hpp>

namespace tuvx
{
  /// @brief Handle for fast quantum yield lookup in warehouse
  ///
  /// QuantumYieldHandle provides O(1) access to quantum yields in the warehouse
  /// after initial name-based lookup.
  class QuantumYieldHandle
  {
   public:
    /// @brief Default constructor creates an invalid handle
    QuantumYieldHandle() = default;

    /// @brief Check if this handle is valid
    bool IsValid() const
    {
      return valid_;
    }

    /// @brief Get the index for direct access
    std::size_t Index() const
    {
      return index_;
    }

   private:
    friend class QuantumYieldWarehouse;

    explicit QuantumYieldHandle(std::size_t index) : index_(index), valid_(true)
    {
    }

    std::size_t index_{ 0 };
    bool valid_{ false };
  };

  /// @brief Collection manager for quantum yields
  ///
  /// QuantumYieldWarehouse stores quantum yields and provides both name-based
  /// and handle-based access. Quantum yields are stored by unique_ptr and
  /// owned by the warehouse.
  class QuantumYieldWarehouse
  {
   public:
    QuantumYieldWarehouse() = default;

    // Prevent copying
    QuantumYieldWarehouse(const QuantumYieldWarehouse&) = delete;
    QuantumYieldWarehouse& operator=(const QuantumYieldWarehouse&) = delete;

    // Allow moving
    QuantumYieldWarehouse(QuantumYieldWarehouse&&) = default;
    QuantumYieldWarehouse& operator=(QuantumYieldWarehouse&&) = default;

    /// @brief Add a quantum yield to the warehouse
    /// @param quantum_yield The quantum yield to add (takes ownership)
    /// @return Handle for fast subsequent access
    /// @throws std::runtime_error if a quantum yield with the same name exists
    QuantumYieldHandle Add(std::unique_ptr<QuantumYield> quantum_yield)
    {
      if (!quantum_yield)
      {
        throw std::runtime_error("Cannot add null quantum yield to warehouse");
      }

      const std::string& name = quantum_yield->Name();
      if (name_to_index_.count(name) > 0)
      {
        throw std::runtime_error("Quantum yield '" + name + "' already exists in warehouse");
      }

      std::size_t index = quantum_yields_.size();
      quantum_yields_.push_back(std::move(quantum_yield));
      name_to_index_[name] = index;

      return QuantumYieldHandle(index);
    }

    /// @brief Get quantum yield by name
    /// @param name The quantum yield name
    /// @return Reference to the quantum yield
    /// @throws std::out_of_range if not found
    const QuantumYield& Get(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Quantum yield '" + name + "' not found in warehouse");
      }
      return *quantum_yields_[it->second];
    }

    /// @brief Get quantum yield by handle (O(1) access)
    /// @param handle A valid handle from Add()
    /// @return Reference to the quantum yield
    /// @throws std::out_of_range if handle is invalid
    const QuantumYield& Get(QuantumYieldHandle handle) const
    {
      if (!handle.IsValid() || handle.Index() >= quantum_yields_.size())
      {
        throw std::out_of_range("Invalid quantum yield handle");
      }
      return *quantum_yields_[handle.Index()];
    }

    /// @brief Get handle for a quantum yield by name
    /// @param name The quantum yield name
    /// @return Handle for fast access
    /// @throws std::out_of_range if not found
    QuantumYieldHandle GetHandle(const std::string& name) const
    {
      auto it = name_to_index_.find(name);
      if (it == name_to_index_.end())
      {
        throw std::out_of_range("Quantum yield '" + name + "' not found in warehouse");
      }
      return QuantumYieldHandle(it->second);
    }

    /// @brief Check if a quantum yield exists
    /// @param name The quantum yield name
    /// @return True if quantum yield exists
    bool Exists(const std::string& name) const
    {
      return name_to_index_.count(name) > 0;
    }

    /// @brief Get all quantum yield names
    /// @return Vector of names in insertion order
    std::vector<std::string> Names() const
    {
      std::vector<std::string> names;
      names.reserve(quantum_yields_.size());
      for (const auto& qy : quantum_yields_)
      {
        names.push_back(qy->Name());
      }
      return names;
    }

    /// @brief Get the number of quantum yields
    std::size_t Size() const
    {
      return quantum_yields_.size();
    }

    /// @brief Check if warehouse is empty
    bool Empty() const
    {
      return quantum_yields_.empty();
    }

    /// @brief Clear all quantum yields
    void Clear()
    {
      quantum_yields_.clear();
      name_to_index_.clear();
    }

   private:
    std::vector<std::unique_ptr<QuantumYield>> quantum_yields_;
    std::unordered_map<std::string, std::size_t> name_to_index_;
  };

}  // namespace tuvx
