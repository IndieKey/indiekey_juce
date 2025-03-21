//
// Created by Ruurd Adema on 28/07/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

#include "Activation.h"
#include "ActivationsDatabase.h"
#include "ProductData.h"
#include "RestClient.h"

#include <juce_core/juce_core.h>

namespace indiekey
{

class ActivationClient
{
public:
    enum class ValidationStrategy
    {
        /// Validate without contacting the server. If no activations are available, no activations are loaded.
        LocalOnly,

        /// Validate without contacting the server. If no *valid* activations are available, no activations are loaded.
        LocalValidOnly,

        /// Validate by contacting the server for the activations which require an update.
        Online,

        /// Validate by contacting the server and update all activations.
        ForceOnline
    };

    enum class TrialStatus
    {
        /// Default undefined status.
        Undefined,

        /// Status for when a trial is available but not active.
        TrialAvailable,

        /// Status for when a trial is active.
        TrialActive,

        /// Status for when a trial is expired and can no longer be used on this machine.
        TrialExpired,
    };

    /**
     * Baseclass for other classes who are interested in updates from an ActivationClient.
     */
    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;

        /**
         * Called in response to calling subscribeToActivationClient and validate and contains the currently loaded
         * activation, or nullptr if no activation is loaded. It the latter is the case then no activations were
         * available at all.
         * @param mostValuableActivation The loaded most valuable activation or nullptr of no activation is available.
         * @package trialActivationExists True if a trial activation exists, false otherwise.
         */
        virtual void onActivationsUpdated ([[maybe_unused]] const Activation* mostValuableActivation) {}
    };

    explicit ActivationClient();

    /**
     * Provides the product data to the activation client. This data is used to validate activations.
     * @param encodedProductData ProductData encoded as bade64 standard padded.
     */
    void setProductData (const char* encodedProductData);

    /**
     * @returns The currently set product data, or nullptr if no data is set/
     */
    [[nodiscard]] const ProductData* getProductData() const;

    /**
     * Sets device info which will be attached to activations for easier identification. This is optional and not used
     * by IndieKey itself. The result from getDefaultDeviceInfo is set as default. For offline requests this data will
     * be encrypted.
     * @param deviceInfo The device info to set.
     */
    [[maybe_unused]] void setDeviceInfo (std::optional<std::string>&& deviceInfo);

    /**
     * Invokes a validation of the most valuable activation.
     * @param validationStrategy The validation strategy to use.
     * @throws std::runtime_error If an error occurs during validation.
     */
    void validate (ValidationStrategy validationStrategy);

    /**
     * Tries to activate the product with given email address and serial key.
     * @param emailAddress The email address.
     * @param licenseKey The serial key.
     */
    void activate (const std::string& emailAddress, const std::string& licenseKey);

    /**
     * Tries to start a trial for this product with given email address.
     * @param emailAddress The email address.
     */
    void startTrial (const std::string& emailAddress);

    /**
     * Saves the activation request to given file. The file can be used to activate the product on another machine.
     * @param emailAddress The email address of the license.
     * @param licenseKey The license key.
     * @param fileToSaveTo The file to save to. Will overwrite the file if it already exists.
     * @param trial True to save a trial request, false to save a full activation request.
     */
    void saveActivationRequest (
        const std::string& emailAddress,
        const std::string& licenseKey,
        const juce::File& fileToSaveTo,
        bool trial);

    /**
     * Tries to activate the software from given file. File must have been generated on the same server as the product
     * data.
     * @param fileToLoad The file to load.
     */
    void installActivationFile (const juce::File& fileToLoad);

    /**
     * Tries to activate the software from given activation.
     * @param activation The activation to install
     */
    void installActivation (indiekey::Activation&& activation);

    /**
     * Destroys all locally stored activations. It will not contact the server not destroy the online store activations
     * which means that re-activating will re-use the same activation and it will not 'cost' any activations.
     */
    int destroyAllLocalActivations();

    /**
     * @returns The status of the trial on this machine.
     */
    TrialStatus getTrialStatus();

    /**
     * Sends a ping to the server.
     * @param value
     */
    int ping (int value) const;

    /**
     * @returns A default device info string which consists of computer name, operating system name, cpu model, device
     * description and device manufacturer.
     */
    static const std::string& getDefaultDeviceInfo();

    /**
     * @returns The currently loaded activation, or nullptr if no activation is loaded.
     */
    [[nodiscard]] const Activation* getCurrentLoadedActivation() const;

    /**
     * This function returns the activation status of the client.
     * @returns The activation status of the currently loaded activation, or Status::NoActivationLoaded if no activation
     * is loaded.
     */
    [[nodiscard]] Activation::Status getActivationStatus() const;

    /**
     * @returns The path to the file where local activations are stored.
     */
    [[nodiscard]] juce::File getLocalActivationsDatabaseFile() const;

    /**
     * Adds given subscriber to the list of subscribers. The subscriber will be notified when the activation client
     * changes. Initially the subscriber will be notified with the current state.
     * @param subscriber The Listener to add.
     */
    void addListener (Subscriber* subscriber);

    /**
     * Removes given listener from the list of listeners.
     * @param subscriber The listener to remove.
     */
    void removeListener (Subscriber* subscriber);

    /**
     * @param status The status to get a string for.
     * @returns A string representation of the given trial status.
     */
    static const char* trialStatusToString (TrialStatus status);

private:
    std::unique_ptr<RestClient> restClient_;
    std::unique_ptr<ProductData> productData_;
    juce::ListenerList<Subscriber> listeners_;
    std::unique_ptr<Activation> mostValuableActivation_;
    ActivationsDatabase activationsDatabase_;
    std::optional<std::string> deviceInfo_ { getDefaultDeviceInfo() };

    // TODO: Reuse the machine id in some way.
    static std::vector<uint8_t> getUniqueMachineId();
    // TODO: Reuse the machine id in some way.
    static std::string getUniqueMachineIdAsBase64();

    void updateActivations (ValidationStrategy validationStrategy);
    std::vector<Activation> getAllActivationsWhichNeedToBeUpdated (bool forceUpdate);

    /**
     * @returns Runs through given vector and returns iterator to the most valuable activation, or a past-the-end
     * iterator if no activation is available.
     */
    std::vector<Activation>::const_iterator findMostValuableActivation (const std::vector<Activation>& activations);

    void throwIfProductDataIsNotSet() const;
};

} // namespace indiekey
