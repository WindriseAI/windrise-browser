diff --git a/components/search_engines/default_search_manager_unittest.cc b/components/search_engines/default_search_manager_unittest.cc
--- a/components/search_engines/default_search_manager_unittest.cc
+++ b/components/search_engines/default_search_manager_unittest.cc
@@ -249,40 +249,27 @@
   EXPECT_EQ(DefaultSearchManager::FROM_FALLBACK, source);
 }
 
-// Test that DefaultSearch manager detects changes to kSearchProviderOverrides.
-TEST_F(DefaultSearchManagerTest, DefaultSearchSetByOverrides) {
-  base::test::ScopedFeatureList feature_list;
-  feature_list.InitAndDisableFeature(switches::kIgnoreSearchProviderOverrides);
-
-  SetOverrides(pref_service(), false);
-  auto manager = create_manager();
-
-  std::unique_ptr<TemplateURLData> fallback_t_url_data =
-      prepopulate_data_resolver().GetFallbackSearch();
-  EXPECT_NE(fallback_t_url_data->keyword(),
-            TemplateURLPrepopulateData::google.keyword);
-  EXPECT_NE(fallback_t_url_data->prepopulate_id,
-            TemplateURLPrepopulateData::google.id);
-
-  DefaultSearchManager::Source source = DefaultSearchManager::FROM_POLICY;
-  EXPECT_TRUE(manager->GetDefaultSearchEngine(&source));
-  TemplateURLData first_default(*manager->GetDefaultSearchEngine(&source));
-  ExpectSimilar(fallback_t_url_data.get(), &first_default);
-  EXPECT_EQ(DefaultSearchManager::FROM_FALLBACK, source);
-
-  // Update the overrides:
-  SetOverrides(pref_service(), true);
-  fallback_t_url_data = prepopulate_data_resolver().GetFallbackSearch();
-
-  // Make sure DefaultSearchManager updated:
-  ExpectSimilar(fallback_t_url_data.get(),
-                manager->GetDefaultSearchEngine(&source));
-  EXPECT_EQ(DefaultSearchManager::FROM_FALLBACK, source);
-  EXPECT_NE(manager->GetDefaultSearchEngine(nullptr)->short_name(),
-            first_default.short_name());
-  EXPECT_NE(manager->GetDefaultSearchEngine(nullptr)->keyword(),
-            first_default.keyword());
-}
+// Windrise never substitutes a regional fallback for Google or a user choice.
+TEST_F(DefaultSearchManagerTest, WindriseFallbackAndUserChoice) {
+  base::test::ScopedFeatureList feature_list;
+  feature_list.InitAndDisableFeature(switches::kIgnoreSearchProviderOverrides);
+  SetOverrides(pref_service(), false);
+  auto manager = create_manager();
+  DefaultSearchManager::Source source = DefaultSearchManager::FROM_POLICY;
+  ASSERT_TRUE(manager->GetDefaultSearchEngine(&source));
+  EXPECT_EQ(TemplateURLPrepopulateData::google.id,
+            manager->GetDefaultSearchEngine(&source)->prepopulate_id);
+  EXPECT_EQ(DefaultSearchManager::FROM_FALLBACK, source);
+  SetOverrides(pref_service(), true);
+  EXPECT_EQ(TemplateURLPrepopulateData::google.id,
+            manager->GetDefaultSearchEngine(&source)->prepopulate_id);
+  auto selected = GenerateDummyTemplateURLData("chosen.example");
+  manager->SetUserSelectedDefaultSearchEngine(*selected);
+  manager.reset();
+  manager = create_manager();
+  ExpectSimilar(selected.get(), manager->GetDefaultSearchEngine(&source));
+  EXPECT_EQ(DefaultSearchManager::FROM_USER, source);
+}
 
 // Test DefaultSearchManager handles policy-enforced DSEs correctly.
 TEST_F(DefaultSearchManagerTest, DefaultSearchSetByEnforcedPolicy) {
