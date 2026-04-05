ALTER TABLE project_credentials
    ADD COLUMN ssh_private_key_enc text,
    ADD COLUMN ssh_public_key_enc  text;
