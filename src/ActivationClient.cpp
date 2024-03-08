//
// Created by Ruurd Adema on 28/07/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#include "indiekey/ActivationClient.h"
#include "indiekey/Crypto.h"
#include "indiekey/Endpoints.h"
#include "indiekey/ProductData.h"
#include "indiekey/messages/ActivationRequest.h"
#include "indiekey/messages/OfflineRequest.h"
#include "indiekey/messages/TrialRequest.h"

indiekey::ActivationClient::ActivationClient()
{
    crypto::init();
}

void indiekey::ActivationClient::ping (int value)
{
    nlohmann::json j;
    j.at ("id") = value;

    auto response = restClient_->post ("/ping", j);

    response.throwIfNotSuccessful();

    auto jsonResponse = nlohmann::json::parse (response.body.toRawUTF8());

    std::cout << response.statusCode << " " << jsonResponse.dump() << std::endl;
}

void indiekey::ActivationClient::setProductData (const char* encodedProductData)
{
    if (encodedProductData == nullptr)
        throw std::runtime_error ("Product data is invalid");

    if (std::strlen (encodedProductData) == 0)
        throw std::runtime_error ("Product data is empty");

    nlohmann::json jsonData = nlohmann::json::parse (decodeFromBase64 (encodedProductData));

    if (productData_ == nullptr)
        productData_ = std::make_unique<ProductData>();

    *productData_ = jsonData.get<ProductData>();

    restClient_ = std::make_unique<RestClient> (juce::URL (productData_->primaryPublicServerAddress));

    activationsDatabase_.openDatabase (ActivationsDatabase::Options { getLocalActivationsDatabaseFile() });
}

void indiekey::ActivationClient::validate (ValidationStrategy validationStrategy)
{
    throwIfProductDataIsNotSet();

    mostValuableActivation_.reset();

    updateActivations (validationStrategy);

    auto activations = activationsDatabase_.getActivations (productData_->productUid, getUniqueMachineId());

    auto mostValuableActivation = findMostValuableActivation (activations);

    if (mostValuableActivation != activations.end())
    {
        auto activation = std::make_unique<Activation> (*mostValuableActivation);
        auto status = activation->validate (productData_->productUid, getUniqueMachineId(), productData_->verifyingKey);

        // When the strategy is ValidationStrategy::LocalValidOnly we only store the activation when it is valid in
        // order to allow a first, quick check without triggering warnings when an activation is not valid.
        if (validationStrategy != ValidationStrategy::LocalValidOnly || status == Activation::Status::Valid)
            mostValuableActivation_ = std::move (activation);
    }

    listeners_.call ([this] (Subscriber& s) {
        s.onActivationsUpdated (mostValuableActivation_.get());
    });
}

void indiekey::ActivationClient::activate (const std::string& emailAddress, const std::string& licenseKey)
{
    throwIfProductDataIsNotSet();

    if (emailAddress.empty())
        throw std::runtime_error ("Email address is empty");

    if (licenseKey.empty())
        throw std::runtime_error ("License key is empty");

    ActivationRequest activationRequest (
        productData_->productUid,
        getUniqueMachineIdAsBase64(),
        emailAddress,
        licenseKey,
        deviceInfo_);

    auto response = restClient_->post (ENDPOINT_ACTIVATE, activationRequest);
    response.throwIfNotSuccessful();
    installActivation (nlohmann::json::parse (response.body.toRawUTF8()).get<Activation>());
}

std::vector<uint8_t> indiekey::ActivationClient::getUniqueMachineId()
{
    auto uniqueId = juce::SystemStats::getUniqueDeviceID().toStdString();

    if (uniqueId.empty())
        throw std::runtime_error ("Failed to get unique machine id");

    return crypto::genericHash (uniqueId);
}

std::string indiekey::ActivationClient::getUniqueMachineIdAsBase64()
{
    return encodeToBase64 (getUniqueMachineId());
}

void indiekey::ActivationClient::updateActivations (ValidationStrategy validationStrategy)
{
    throwIfProductDataIsNotSet();

    if (validationStrategy == ValidationStrategy::LocalOnly || validationStrategy == ValidationStrategy::LocalValidOnly)
        return; // Nothing to do here.

    auto requestActivations = getAllActivationsWhichNeedToBeUpdated (
        validationStrategy == ValidationStrategy::ForceOnline);

    if (requestActivations.empty())
        return; // Nothing to do at this moment.

    auto response = restClient_->post (ENDPOINT_UPDATE_ACTIVATIONS, requestActivations);
    response.throwIfNotSuccessful();
    auto responseActivations = nlohmann::json::parse (response.body.toRawUTF8()).get<std::vector<Activation>>();

    for (auto& activation : responseActivations)
        activationsDatabase_.saveActivation (activation);

    // Delete all activations from local disk which are not in the response.
    for (auto& requestActivation : requestActivations)
    {
        bool found = false;

        for (auto& responseActivation : responseActivations)
        {
            if (requestActivation.getHash() == responseActivation.getHash())
            {
                found = true;
                break;
            }
        }

        if (!found)
            activationsDatabase_.deleteActivation (requestActivation.getHash());
    }
}

std::vector<indiekey::Activation> indiekey::ActivationClient::getAllActivationsWhichNeedToBeUpdated (bool forceUpdate)
{
    throwIfProductDataIsNotSet();

    return activationsDatabase_.getActivationsWhichNeedUpdate (
        productData_->productUid,
        getUniqueMachineId(),
        forceUpdate);
}

std::vector<indiekey::Activation, std::allocator<indiekey::Activation>>::const_iterator indiekey::ActivationClient::
    findMostValuableActivation (const std::vector<Activation>& activations)
{
    throwIfProductDataIsNotSet();

    if (activations.empty())
        return activations.cend();

    auto mostValuableActivation = activations.cbegin();

    for (auto it = activations.cbegin(); it != activations.cend(); ++it)
    {
        if (it->isMoreValuableThan (*mostValuableActivation))
            mostValuableActivation = it;
    }

    return mostValuableActivation;
}

int indiekey::ActivationClient::destroyAllLocalActivations()
{
    throwIfProductDataIsNotSet();

    return activationsDatabase_.deleteAllActivations (productData_->productUid, getUniqueMachineId());
}

void indiekey::ActivationClient::startTrial (const std::string& emailAddress)
{
    throwIfProductDataIsNotSet();

    TrialRequest trialRequest (productData_->productUid, getUniqueMachineIdAsBase64(), emailAddress, deviceInfo_);

    auto response = restClient_->post (ENDPOINT_ACTIVATE_TRIAL, trialRequest);
    response.throwIfNotSuccessful();

    installActivation (nlohmann::json::parse (response.body.toRawUTF8()).get<Activation>());
}

void indiekey::ActivationClient::saveActivationRequest (
    const std::string& emailAddress,
    const std::string& licenseKey,
    const juce::File& fileToSaveTo,
    bool trial)
{
    throwIfProductDataIsNotSet();

    OfflineRequest offlineRequest;

    std::optional<std::string> deviceInfo = std::nullopt;

    if (deviceInfo_)
        deviceInfo = encodeToBase64 (crypto::boxSeal (deviceInfo_.value(), productData_->cryptoPublicKey));

    if (trial)
    {
        offlineRequest.trialRequest = OfflineRequest::TrialRequest (
            productData_->productUid,
            getUniqueMachineIdAsBase64(),
            encodeToBase64 (crypto::boxSeal (emailAddress, productData_->cryptoPublicKey)),
            deviceInfo);
    }
    else
    {
        offlineRequest.activationRequest = OfflineRequest::ActivationRequest (
            productData_->productUid,
            getUniqueMachineIdAsBase64(),
            encodeToBase64 (crypto::boxSeal (emailAddress, productData_->cryptoPublicKey)),
            encodeToBase64 (crypto::boxSeal (licenseKey, productData_->cryptoPublicKey)),
            deviceInfo);
    }

    auto dump = nlohmann::json (offlineRequest).dump();

    if (!fileToSaveTo.replaceWithData (dump.data(), dump.size()))
        throw std::runtime_error ("Failed to save activation request");
}

void indiekey::ActivationClient::installActivationFile (const juce::File& fileToLoad)
{
    auto json = fileToLoad.loadFileAsString();

    if (json.isEmpty())
        throw std::runtime_error ("Failed to load activation file");

    try
    {
        auto activation = nlohmann::json::parse (json.toRawUTF8()).get<Activation>();
        installActivation (std::move (activation));
    }
    catch (const std::exception& originalException)
    {
        // Before throwing the original exception, try to parse the file as an offline request to see if the user
        // accidentally tries to load a request file instead of a response file.
        try
        {
            (void)nlohmann::json::parse (json.toRawUTF8()).get<OfflineRequest>();
        }
        catch (const std::exception&)
        {
            // Parsing as OfflineRequest failed so something else is going on.
            throw std::runtime_error (originalException.what());
        }

        // Parsing as OfflineRequest succeeded so this is likely a request file.
        throw std::runtime_error ("This is a request file. Please install a response file.");
    }
}

indiekey::ActivationClient::TrialStatus indiekey::ActivationClient::getTrialStatus()
{
    throwIfProductDataIsNotSet();

    auto trialActivations = activationsDatabase_.getTrialActivations (productData_->productUid, getUniqueMachineId());

    auto mostValuableTrialActivation = findMostValuableActivation (trialActivations);

    if (mostValuableTrialActivation == trialActivations.end())
        return TrialStatus::TrialAvailable; // No trial yet exists which means it is still available.

    if (mostValuableTrialActivation->isExpired())
        return TrialStatus::TrialExpired;

    return TrialStatus::TrialActive;
}

const indiekey::ProductData* indiekey::ActivationClient::getProductData() const
{
    return productData_.get();
}

void indiekey::ActivationClient::throwIfProductDataIsNotSet() const
{
    if (productData_ == nullptr)
        throw std::runtime_error ("Product data not set");
}

void indiekey::ActivationClient::installActivation (indiekey::Activation&& activation)
{
    throwIfProductDataIsNotSet();

    auto status = activation.validate (productData_->productUid, getUniqueMachineId(), productData_->verifyingKey);

    if (status != Activation::Status::Valid)
        throw std::runtime_error (std::string ("Activation failed: ") + Activation::statusToString (status));

    activationsDatabase_.saveActivation (activation);

    validate (ValidationStrategy::Online);
}

const std::string& indiekey::ActivationClient::getDefaultDeviceInfo()
{
    static const auto stats = (juce::SystemStats::getComputerName() + ", " +
                               juce::SystemStats::getOperatingSystemName() + ", " + juce::SystemStats::getCpuModel() +
                               ", " + juce::SystemStats::getDeviceDescription() + ", " +
                               juce::SystemStats::getDeviceManufacturer())
                                  .toStdString();
    return stats;
}

void indiekey::ActivationClient::setDeviceInfo (std::optional<std::string>&& deviceInfo)
{
    deviceInfo_ = deviceInfo;
}

const indiekey::Activation* indiekey::ActivationClient::getCurrentLoadedActivation() const
{
    return mostValuableActivation_.get();
}

indiekey::Activation::Status indiekey::ActivationClient::getActivationStatus() const
{
    if (mostValuableActivation_ == nullptr)
        return indiekey::Activation::Status::NoActivationLoaded;
    return mostValuableActivation_->getStatus();
}

juce::File indiekey::ActivationClient::getLocalActivationsDatabaseFile() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
#ifdef JUCE_MAC
        .getChildFile ("Application Support")
#endif
        .getChildFile (productData_->organisationName)
        .getChildFile ("activations.db");
}

void indiekey::ActivationClient::addListener (indiekey::ActivationClient::Subscriber* subscriber)
{
    if (subscriber == nullptr)
        return;

    subscriber->onActivationsUpdated (mostValuableActivation_.get());
    listeners_.add (subscriber);
}

void indiekey::ActivationClient::removeListener (indiekey::ActivationClient::Subscriber* subscriber)
{
    listeners_.remove (subscriber);
}
