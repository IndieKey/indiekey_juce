//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#include "indiekey/ActivationsDatabase.h"

bool indiekey::ActivationsDatabase::Options::operator== (const ActivationsDatabase::Options& rhs) const
{
    return databaseFile == rhs.databaseFile;
}

bool indiekey::ActivationsDatabase::Options::operator!= (const ActivationsDatabase::Options& rhs) const
{
    return !(rhs == *this);
}

void indiekey::ActivationsDatabase::openDatabase (const ActivationsDatabase::Options& options)
{
    if (options_ == options)
        return;

    // Open new database if necessary
    if (options_.databaseFile != options.databaseFile)
    {
        // Make sure the path is legal
        jassert (
            options.databaseFile.getFullPathName() ==
            juce::File::createLegalPathName (options.databaseFile.getFullPathName()));

        auto result = options.databaseFile.getParentDirectory().createDirectory();
        if (result.failed())
            throw std::runtime_error (result.getErrorMessage().toStdString());

        database_.reset();

        options_.databaseFile = options.databaseFile;

        database_ = std::make_unique<SQLite::Database> (
            options_.databaseFile.getFullPathName().toRawUTF8(),
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE,
            kBusyTimeoutMs);

        migrate();
    }

    options_ = options;
}

void indiekey::ActivationsDatabase::migrate()
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    SQLite::Statement statement (
        *database_,
        R"(create table if not exists activations(
            id                 integer primary key autoincrement,
            hash               blob unique not null,
            product_uid        text        not null,
            machine_uid        blob        not null,
            expires_at         integer,
            license_expires_at integer,
            last_updated_at    integer     not null,
            license_type       text        not null,
            signature          blob        not null);
        )");

    statement.exec();
}

void indiekey::ActivationsDatabase::saveActivation (const indiekey::Activation& activation)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    auto now = juce::Time::getCurrentTime();

    SQLite::Statement statement (
        *database_,
        R"(INSERT OR REPLACE INTO activations(
            hash, product_uid, machine_uid, expires_at, license_expires_at, last_updated_at, license_type, signature)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?);
        )");

    const auto& activationHash = activation.getHash();
    statement.bind (1, activationHash.data(), static_cast<int> (activationHash.size()));
    statement.bind (2, activation.getProductUid());
    const auto& machineUid = activation.getMachineUid();
    statement.bind (3, machineUid.data(), static_cast<int> (machineUid.size()));
    activation.getExpiresAt().has_value() ? statement.bind (4, activation.getExpiresAt()->toMilliseconds())
                                          : statement.bind (4, nullptr);
    activation.getLicenseExpiresAt().has_value()
        ? statement.bind (5, activation.getLicenseExpiresAt()->toMilliseconds())
        : statement.bind (5, nullptr);
    statement.bind (6, now.toMilliseconds());
    statement.bind (7, License::typeToString (activation.getLicenseType()));
    const auto& signature = activation.getSignature();
    statement.bind (8, signature.data(), static_cast<int> (signature.size()));

    statement.exec();
}

void indiekey::ActivationsDatabase::deleteActivation (const indiekey::Activation::Hash& activationHash)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    SQLite::Statement statement (*database_, "DELETE FROM activations WHERE hash = ?");
    statement.bind (1, activationHash.data(), static_cast<int> (activationHash.size()));
    statement.exec();
}

static std::vector<uint8_t> toBlobVector (const SQLite::Column& column)
{
    if (!column.isBlob())
        return {};
    auto size = column.getBytes();
    auto data = static_cast<const uint8_t*> (column.getBlob());
    return { data, data + size };
}

[[maybe_unused]] static juce::Time toTime (const SQLite::Column& column)
{
    return juce::Time (column);
}

static std::optional<juce::Time> toOptionalTime (const SQLite::Column& column)
{
    return column.isNull() ? std::optional<juce::Time> {} : std::optional<juce::Time> { column };
}

namespace
{

indiekey::Activation getActivationFromQuery (SQLite::Statement& query)
{
    return { toBlobVector (query.getColumn ("hash")),
             query.getColumn ("product_uid").getString(),
             toBlobVector (query.getColumn ("machine_uid")),
             toOptionalTime (query.getColumn ("expires_at")),
             toOptionalTime (query.getColumn ("license_expires_at")),
             indiekey::License::typeFromString (query.getColumn ("license_type")),
             toBlobVector (query.getColumn ("signature")) };
}
} // namespace

std::vector<indiekey::Activation> indiekey::ActivationsDatabase::getActivations (
    const std::string& productUid,
    const std::vector<uint8_t>& machineUid)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    SQLite::Statement query (
        *database_,
        R"(SELECT hash, product_uid, machine_uid, expires_at, license_expires_at, license_type, signature
             FROM activations
            WHERE product_uid = ? AND machine_uid = ?
        )");

    query.bind (1, productUid);
    query.bind (2, machineUid.data(), static_cast<int> (machineUid.size()));

    std::vector<Activation> activations;

    while (query.executeStep())
        activations.emplace_back (getActivationFromQuery (query));

    return activations;
}

std::vector<indiekey::Activation> indiekey::ActivationsDatabase::getTrialActivations (
    const std::string& productUid,
    const std::vector<uint8_t>& machineUid)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    SQLite::Statement query (
        *database_,
        R"(SELECT hash, product_uid, machine_uid, expires_at, license_expires_at, license_type, signature
             FROM activations
            WHERE product_uid = ? AND machine_uid = ? AND license_type = ?
        )");

    query.bind (1, productUid);
    query.bind (2, machineUid.data(), static_cast<int> (machineUid.size()));
    query.bind (3, License::typeToString (License::Type::Trial));

    std::vector<Activation> activations;

    while (query.executeStep())
        activations.emplace_back (getActivationFromQuery (query));

    return activations;
}

int indiekey::ActivationsDatabase::deleteAllActivations (
    const std::string& productUid,
    const std::vector<uint8_t>& machineUid)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    SQLite::Statement query (
        *database_,
        R"(DELETE FROM activations
           WHERE product_uid = ? AND machine_uid = ?;
        )");

    query.bind (1, productUid);
    query.bind (2, machineUid.data(), static_cast<int> (machineUid.size()));
    return query.exec();
}

std::vector<indiekey::Activation> indiekey::ActivationsDatabase::getActivationsWhichNeedUpdate (
    const std::string& productUid,
    const std::vector<uint8_t>& machineUid,
    bool getAllActivations)
{
    if (database_ == nullptr)
        throw std::runtime_error ("Database not open");

    // TODO: Make this configurable as part of the activation returned by the server.
    static constexpr int onlineCheckIntervalHours = 24;
    auto now = juce::Time::getCurrentTime();

    SQLite::Statement query (
        *database_,
        R"(SELECT hash, product_uid, machine_uid, expires_at, license_expires_at, license_type, signature
             FROM activations
            WHERE product_uid = ? AND machine_uid = ? AND (expires_at < ? OR last_updated_at < ? OR ?)
        )");

    // Note: we don't have to test for license_expires_at because expires_at will (should) never outlast
    // license_expires_at.

    query.bind (1, productUid);
    query.bind (2, machineUid.data(), static_cast<int> (machineUid.size()));
    query.bind (3, (now + juce::RelativeTime::hours (onlineCheckIntervalHours)).toMilliseconds());
    query.bind (4, (now - juce::RelativeTime::hours (onlineCheckIntervalHours)).toMilliseconds());
    query.bind (5, getAllActivations);

    std::vector<Activation> activations;

    while (query.executeStep())
        activations.emplace_back (getActivationFromQuery (query));

    return activations;
}
