//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#pragma once

#include "Activation.h"
#include <SQLiteCpp/Database.h>
#include <juce_core/juce_core.h>
#include <string>

namespace indiekey
{

class ActivationsDatabase
{
public:
    /**
     * Specify different options which influence the location and name of the database.
     */
    struct Options
    {
        juce::File databaseFile;

        bool operator== (const Options& rhs) const;
        bool operator!= (const Options& rhs) const;
    };

    /**
     * Sets the options for the database. If the options are different from the current options, the database will be
     * (re)opened.
     * @param options The options to use for opening the database.
     */
    void openDatabase (const Options& options);

    /**
     * Brings the database in a state which is compatible with the current version of the application.
     */
    void migrate();

    /**
     * Save given activation to the database.
     * @param activation Activation to save.
     */
    void saveActivation (const Activation& activation);

    /**
     * Delete activation for given hash from the database.
     * @param activationHash The hash of the activation to delete.
     */
    void deleteActivation (const Activation::Hash& activationHash);

    /**
     * Delete all activations for given product uid and machine uid.
     * @param productUid The product uid of the activations to delete.
     * @param machineUid The machine uid of the activations to delete.
     */
    int deleteAllActivations (const std::string& productUid, const std::vector<uint8_t>& machineUid);

    /**
     * Finds all activations for given product uid and machine uid.
     * @param productUid The product uid to search for.
     * @param machineUid The machine uid to search for.
     */
    std::vector<Activation> getActivations (const std::string& productUid, const std::vector<uint8_t>& machineUid);

    /**
     * Find all trial activations for given product uid and machine uid.
     * @param productUid
     * @param machineUid
     * @return
     */
    std::vector<Activation> getTrialActivations (const std::string& productUid, const std::vector<uint8_t>& machineUid);

    /**
     * Finds all activations for given product uid and machine uid which need to be updated. Activations are considered
     * for updating when the expiration date is within a day from now or when it was last updated more than a day ago.
     * @param productUid The product uid to search for.
     * @param machineUid The machine uid to search for.
     * @param getAllActivations If true, all activations will be returned, otherwise only activations which need to be updated
     * will be returned.
     * @return A vector of activations which need to be updated.
     */
    std::vector<indiekey::Activation> getActivationsWhichNeedUpdate (
        const std::string& productUid,
        const std::vector<uint8_t>& machineUid,
        bool getAllActivations);

private:
    static constexpr int kBusyTimeoutMs = 1000;
    Options options_;
    std::unique_ptr<SQLite::Database> database_;
};

} // namespace indiekey
