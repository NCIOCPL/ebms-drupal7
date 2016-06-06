CREATE TABLE ebms_topic_group
    (group_id INTEGER         NOT NULL AUTO_INCREMENT PRIMARY KEY,
   group_name VARCHAR(255)    NOT NULL UNIQUE,
active_status ENUM ('A', 'I') NOT NULL DEFAULT 'A')
ENGINE=InnoDB DEFAULT CHARSET=utf8;
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Breast Cancer');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Digestive/Gastrointestinal Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Endocrine Gland Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Genitourinary Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Gynecologic Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Head and Neck Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Thorax/Respiratory Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Hematologic and Lymph Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('AIDS-Related Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Skin Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Neurologic Cancers');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Eye Cancer');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Musculoskeletal Cancer');
INSERT INTO ebms_topic_group (group_name)
     VALUES ('Other');
ALTER TABLE ebms_topic
    ADD topic_group INTEGER NULL REFERENCES ebms_topic_group;
ALTER TABLE ebms_topic
    ADD FOREIGN KEY (topic_group)
    REFERENCES ebms_topic_group (group_id);
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Breast Cancer')
 WHERE topic_name = 'Breast Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Breast Cancer')
 WHERE topic_name = 'Breast Cancer and Pregnancy';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Breast Cancer')
 WHERE topic_name = 'Male Breast Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Adult Primary Liver Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Anal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Bile Duct Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Colon Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Colorectal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Esophageal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Gastrointestinal Carcinoid Tumor';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Gallbladder Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Gastric Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Pancreatic Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name LIKE 'Pancreatic Neuroendocrine Tumors (Islet Cell Tumors)%';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Rectal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Digestive/Gastrointestinal Cancers')
 WHERE topic_name = 'Small Intestine Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Endocrine Gland Cancers')
 WHERE topic_name = 'Adrenocortical Carcinoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Endocrine Gland Cancers')
 WHERE topic_name = 'Parathyroid Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Endocrine Gland Cancers')
 WHERE topic_name = 'Pheochromocytoma and Paraganglioma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Bladder Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Extragonadal Germ Cell Tumors';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Prostate Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Penile Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Renal Cell Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Testicular Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Transitional Cell Cancer of the Renal Pelvis and Ureter';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Genitourinary Cancers')
 WHERE topic_name = 'Urethral Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Cervical Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Endometrial Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Gestational Trophoblastic Tumors and Neoplasia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Ovarian Epithelial Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Ovarian Germ Cell Tumors';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Ovarian Low Malignant Potential Tumors';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Uterine Sarcoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Vaginal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Gynecologic Cancers')
 WHERE topic_name = 'Vulval Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Head and Neck';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Hypopharyngeal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Laryngeal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Lip and Oral Cavity Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Metastatic Squamous Neck Cancer with Occult Primary';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Nasopharyngeal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Oropharyngeal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Paranasal Sinus and Nasal Cavity Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Pharyngeal Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Salivary Gland Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Head and Neck Cancers')
 WHERE topic_name = 'Thyroid Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Thorax/Respiratory Cancers')
 WHERE topic_name = 'Lung Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Thorax/Respiratory Cancers')
 WHERE topic_name = 'Malignant Mesothelioma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Thorax/Respiratory Cancers')
 WHERE topic_name = 'Non-Small Cell Lung Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Thorax/Respiratory Cancers')
 WHERE topic_name = 'Small Cell Lung Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Thorax/Respiratory Cancers')
 WHERE topic_name = 'Thyoma and Thymic Carcinoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Acute Lymphoblastic Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Acute Myeloid Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Hodgkin Lymphoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Lymphoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Adult Non-Hodgkin Lymphoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Chronic Lymphocytic Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Chronic Myelogenous Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Chronic Myeloproliferative Neoplasms';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Hairy Cell Leukemia';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Plasma Cell Neoplasms (Including Multiple Myeloma)';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Mycosis Fungoides/Sezary Syndrome';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Myelodysplastic Syndromes';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Myelodysplastic/Myeloproliferative Neoplasms';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Hematologic and Lymph Cancers')
 WHERE topic_name = 'Primary Central Nervous System Lymphoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'AIDS-Related Cancers')
 WHERE topic_name = 'AIDS-Related Lymphoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'AIDS-Related Cancers')
 WHERE topic_name = 'Kaposi Sarcoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Skin Cancers')
 WHERE topic_name = 'Melanoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Skin Cancers')
 WHERE topic_name = 'Merkel Cell Carcinoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Skin Cancers')
 WHERE topic_name = 'Skin Cancer';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Neurologic Cancers')
 WHERE topic_name = 'Adult Central Nervous System Tumors';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Neurologic Cancers')
 WHERE topic_name = 'Pituitary Tumors';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Eye Cancer')
 WHERE topic_name = 'Intraocular (Uveal) Melanoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Musculoskeletal Cancer')
 WHERE topic_name = 'Adult Soft Tissue Sarcoma';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Other')
 WHERE topic_name = 'Carcinoma of Unknown Primary';
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Other')
 WHERE topic_name IN ('Financial Toxicity of Cancer Care',
                      'Cost of Cancer Care');
UPDATE ebms_topic
   SET topic_group = (SELECT group_id
                        FROM ebms_topic_group
                       WHERE group_name = 'Other')
 WHERE topic_name = 'General Adult Treatment';
