-- Run this ONCE only when upgrading an existing pre-v1.5 database.
ALTER TABLE runs ADD COLUMN difficulty TEXT NOT NULL DEFAULT 'UNKNOWN';
ALTER TABLE runs ADD COLUMN rewards INTEGER NOT NULL DEFAULT 0;
ALTER TABLE runs ADD COLUMN platformer INTEGER NOT NULL DEFAULT 1;
