        {
            "id" : "xgb_campaign_$ID",
            "weight" : 1,
            "params" : {
              "campaigns_whitelist_file" : "campaign_$ID.whitelist",
              "feature_mapping_file" : "campaign_$ID.features"
              },
            "models" : [
                {
                    "method" : "xgboost",
                    "features_dimension" : 24,
                    "weight" : 1.0,
                    "features" : [ 
                        ["publisher"],
                        ["tag"],
                        ["sizeid"],
                        ["wd"],
                        ["hour"],
                        ["device"],
                        ["campaign"],
                        ["group"],
                        ["ccid"],
                        ["campaign_freq"],
                        ["campaign_freq_log"],
                        ["geoch"],
                        ["userch"],
                    ],
                    "file" : "campaign_$ID.xgb" 
                },
            ],
        },
