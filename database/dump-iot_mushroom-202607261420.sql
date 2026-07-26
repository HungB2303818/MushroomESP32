-- =====================================================
-- DATABASE: IOT MUSHROOM
-- Mushroom cultivation monitoring and control system
-- =====================================================

CREATE DATABASE IF NOT EXISTS `iot_mushroom`
CHARACTER SET utf8mb4
COLLATE utf8mb4_unicode_ci;

USE `iot_mushroom`;

-- Temporarily disable foreign key checks so that
-- the schema can be safely recreated
SET FOREIGN_KEY_CHECKS = 0;


-- =====================================================
-- DROP EXISTING TABLES
-- Child tables must be dropped before parent tables
-- =====================================================

DROP TABLE IF EXISTS `observation`;
DROP TABLE IF EXISTS `datastream`;
DROP TABLE IF EXISTS `sensor`;

DROP TABLE IF EXISTS `task`;
DROP TABLE IF EXISTS `tasking_capability`;
DROP TABLE IF EXISTS `actuator`;


-- =====================================================
-- 1. SENSOR
-- Stores information about physical sensor devices
-- =====================================================

CREATE TABLE `sensor` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `name` VARCHAR(100) NOT NULL,

    `device_code` VARCHAR(50) NOT NULL,

    PRIMARY KEY (`id`),

    CONSTRAINT `uq_sensor_device_code`
        UNIQUE (`device_code`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- =====================================================
-- 2. DATASTREAM
-- Each datastream represents one observed property
-- produced by a sensor
--
-- Examples:
-- SENSOR_DHT11_01 + temperature
-- SENSOR_DHT11_01 + humidity
-- SENSOR_LIGHT_01 + light_level
-- =====================================================

CREATE TABLE `datastream` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `sensor_id` BIGINT UNSIGNED NOT NULL,

    `name` VARCHAR(100) NOT NULL,

    `observed_property` VARCHAR(50) NOT NULL,

    `unit_symbol` VARCHAR(20) NOT NULL,

    `mqtt_topic` VARCHAR(150) NOT NULL,

    PRIMARY KEY (`id`),

    CONSTRAINT `uq_datastream_sensor_property`
        UNIQUE (`sensor_id`, `observed_property`),

    INDEX `idx_datastream_mqtt_topic`
        (`mqtt_topic`),

    CONSTRAINT `fk_datastream_sensor`
        FOREIGN KEY (`sensor_id`)
        REFERENCES `sensor` (`id`)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- =====================================================
-- 3. OBSERVATION
-- Stores measurement data received from datastreams
-- =====================================================

CREATE TABLE `observation` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `datastream_id` BIGINT UNSIGNED NOT NULL,

    `value` DECIMAL(10, 2) NOT NULL,

    `observed_at` DATETIME NOT NULL,

    `received_at` DATETIME NOT NULL
        DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (`id`),

    INDEX `idx_observation_datastream_time`
        (`datastream_id`, `observed_at`),

    CONSTRAINT `fk_observation_datastream`
        FOREIGN KEY (`datastream_id`)
        REFERENCES `datastream` (`id`)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- =====================================================
-- 4. ACTUATOR
-- Stores information about actuator devices
-- =====================================================

CREATE TABLE `actuator` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `name` VARCHAR(100) NOT NULL,

    `device_code` VARCHAR(50) NOT NULL,

    `type` ENUM(
        'PUMP',
        'FAN',
        'LIGHT',
        'HUMIDIFIER'
    ) NOT NULL,

    PRIMARY KEY (`id`),

    CONSTRAINT `uq_actuator_device_code`
        UNIQUE (`device_code`)
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- =====================================================
-- 5. TASKING CAPABILITY
-- Describes the control capabilities of an actuator
--
-- Example:
-- capability_code: ON_OFF
-- allowed_values: ["ON", "OFF"]
-- =====================================================

CREATE TABLE `tasking_capability` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `actuator_id` BIGINT UNSIGNED NOT NULL,

    `capability_code` VARCHAR(50) NOT NULL,

    `allowed_values` JSON NOT NULL,

    `command_topic` VARCHAR(255) NOT NULL,

    `status_topic` VARCHAR(255) NOT NULL,

    PRIMARY KEY (`id`),

    CONSTRAINT `uq_actuator_capability`
        UNIQUE (`actuator_id`, `capability_code`),

    CONSTRAINT `fk_tasking_capability_actuator`
        FOREIGN KEY (`actuator_id`)
        REFERENCES `actuator` (`id`)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- =====================================================
-- 6. TASK
-- Stores actuator control requests and their results
-- =====================================================

CREATE TABLE `task` (
    `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,

    `tasking_capability_id` BIGINT UNSIGNED NOT NULL,

    `target_value` VARCHAR(50) NOT NULL,

    `source` ENUM(
        'USER',
        'SYSTEM',
        'AUTO_LOCAL'
    ) NOT NULL DEFAULT 'USER',

    `status` ENUM(
        'PENDING',
        'COMPLETED',
        'FAILED'
    ) NOT NULL DEFAULT 'PENDING',

    `created_at` DATETIME NOT NULL
        DEFAULT CURRENT_TIMESTAMP,

    `completed_at` DATETIME DEFAULT NULL,

    PRIMARY KEY (`id`),

    INDEX `idx_task_capability`
        (`tasking_capability_id`),

    INDEX `idx_task_status_created_at`
        (`status`, `created_at`),

    CONSTRAINT `fk_task_tasking_capability`
        FOREIGN KEY (`tasking_capability_id`)
        REFERENCES `tasking_capability` (`id`)
        ON DELETE RESTRICT
        ON UPDATE CASCADE
)
ENGINE = InnoDB
DEFAULT CHARACTER SET = utf8mb4
COLLATE = utf8mb4_unicode_ci;


-- Re-enable foreign key checks after all tables
-- have been successfully created
SET FOREIGN_KEY_CHECKS = 1;