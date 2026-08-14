Feedback Type:
Frown (Error)

Error Message:
Fehler

Stack Trace:


Stack Trace Message:
Fehler

Invocation Stack Trace:
   bei Microsoft.Mashup.Host.Document.ExceptionExtensions.GetCurrentInvocationStackTrace()
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo..ctor(String message, Exception exception, Nullable`1 stackTraceInfo, String messageDetail)
   bei Microsoft.Mashup.Client.UI.Shared.IUIHostExtensions.RaiseErrorDialog(IUIHost uiHost, IWindowHandle activeWindow, FeedbackPackageInfo feedbackPackageInfo, Exception e, LocalizedString dialogTitle, LocalizedString dialogMessage, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.Excel.Native.NativeUserFeedbackServices.ReportException(IWindowHandle activeWindow, IUIHost uiHost, FeedbackPackageInfo feedbackPackageInfo, Exception e, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.<>c__DisplayClass14_0.<HandleException>b__0()
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.HandleException(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.HandleImportEvaluationException(ExceptionResult exceptionView, Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.OnGetPreviewResult(PreviewResult preview, Query query, String sourceID, String formulaTitle, Nullable`1 explicitImportDestination, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass65_0.<GetPreviewResult>b__1()
   bei Microsoft.Mashup.Client.Excel.Shim.NativeWorkbookStorageServices.Microsoft.Mashup.Client.Excel.Shim.IDeferredStorageInvoker.InvokeDeferredStorageAction(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.InvokeCoauthAction(IWorkbook workbook, UndoableActionType actionType, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.NotifyGetDataPresence(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass59_0.<InvokeOnWorkbook>b__0(IWorkbook workbook, IWindowContext windowContext)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.InvokeOnWorkbook[T](Func`3 action, T defaultValue)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.<>c__DisplayClass90_0.<OnQuerySettingsResolved>b__0()
   bei Microsoft.Mashup.Host.Document.ExceptionHandlerExtensions.HandleExceptions(IExceptionHandler exceptionHandler, Action action)
   bei System.RuntimeMethodHandle.InvokeMethod(Object target, Object[] arguments, Signature sig, Boolean constructor)
   bei System.Reflection.RuntimeMethodInfo.UnsafeInvokeInternal(Object obj, Object[] parameters, Object[] arguments)
   bei System.Delegate.DynamicInvokeImpl(Object[] args)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackDo(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackHelper(Object obj)
   bei System.Threading.ExecutionContext.RunInternal(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state)
   bei System.Windows.Forms.Control.InvokeMarshaledCallback(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbacks()
   bei System.Windows.Forms.Control.WndProc(Message& m)
   bei System.Windows.Forms.NativeWindow.Callback(IntPtr hWnd, Int32 msg, IntPtr wparam, IntPtr lparam)


InnerException0.Stack Trace Message:
Das SafeHandle wurde geschlossen.

InnerException0.Stack Trace:
   bei Microsoft.Mashup.Evaluator.EvaluationHost.OnException(IEngineHost engineHost, IMessageChannel channel, ExceptionMessage message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.Dispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.ChannelMessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.Dispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.OnMessageWithUnknownChannel(IMessageChannel baseChannel, MessageWithUnknownChannel messageWithUnknownChannel)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.ChannelMessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.Dispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.Interface.IMessageChannelExtensions.WaitFor[T](IMessageChannel channel)
   bei Microsoft.Mashup.Evaluator.RemotePreviewValueSource.PreviewValueSource.WaitFor(Func`1 condition, Boolean disposing)
   bei Microsoft.Mashup.Evaluator.RemotePreviewValueSource.PreviewValueSource.get_TableSource()
   bei Microsoft.Mashup.Evaluator.Interface.TracingPreviewValueSource.get_TableSource()
   bei Microsoft.Mashup.Host.Document.Analysis.PackageDocumentAnalysisInfo.PackagePartitionAnalysisInfo.SetPreviewValue(EvaluationResult2`1 result, Func`1 getStaleSince, Func`1 getSampled)

InnerException0.Invocation Stack Trace:
   bei Microsoft.Mashup.Host.Document.ExceptionExtensions.GetCurrentInvocationStackTrace()
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromException(Exception e, String prefix)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.CreateAdditionalErrorInfo(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo..ctor(String message, Exception exception, Nullable`1 stackTraceInfo, String messageDetail)
   bei Microsoft.Mashup.Client.UI.Shared.IUIHostExtensions.RaiseErrorDialog(IUIHost uiHost, IWindowHandle activeWindow, FeedbackPackageInfo feedbackPackageInfo, Exception e, LocalizedString dialogTitle, LocalizedString dialogMessage, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.Excel.Native.NativeUserFeedbackServices.ReportException(IWindowHandle activeWindow, IUIHost uiHost, FeedbackPackageInfo feedbackPackageInfo, Exception e, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.<>c__DisplayClass14_0.<HandleException>b__0()
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.HandleException(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.HandleImportEvaluationException(ExceptionResult exceptionView, Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.OnGetPreviewResult(PreviewResult preview, Query query, String sourceID, String formulaTitle, Nullable`1 explicitImportDestination, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass65_0.<GetPreviewResult>b__1()
   bei Microsoft.Mashup.Client.Excel.Shim.NativeWorkbookStorageServices.Microsoft.Mashup.Client.Excel.Shim.IDeferredStorageInvoker.InvokeDeferredStorageAction(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.InvokeCoauthAction(IWorkbook workbook, UndoableActionType actionType, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.NotifyGetDataPresence(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass59_0.<InvokeOnWorkbook>b__0(IWorkbook workbook, IWindowContext windowContext)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.InvokeOnWorkbook[T](Func`3 action, T defaultValue)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.<>c__DisplayClass90_0.<OnQuerySettingsResolved>b__0()
   bei Microsoft.Mashup.Host.Document.ExceptionHandlerExtensions.HandleExceptions(IExceptionHandler exceptionHandler, Action action)
   bei System.RuntimeMethodHandle.InvokeMethod(Object target, Object[] arguments, Signature sig, Boolean constructor)
   bei System.Reflection.RuntimeMethodInfo.UnsafeInvokeInternal(Object obj, Object[] parameters, Object[] arguments)
   bei System.Delegate.DynamicInvokeImpl(Object[] args)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackDo(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackHelper(Object obj)
   bei System.Threading.ExecutionContext.RunInternal(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state)
   bei System.Windows.Forms.Control.InvokeMarshaledCallback(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbacks()
   bei System.Windows.Forms.Control.WndProc(Message& m)
   bei System.Windows.Forms.NativeWindow.Callback(IntPtr hWnd, Int32 msg, IntPtr wparam, IntPtr lparam)


InnerException1.Stack Trace Message:
Das SafeHandle wurde geschlossen.

InnerException1.Stack Trace:
   bei Microsoft.Mashup.Evaluator.EvaluationHost.<>c__DisplayClass24_0.<TryReportException>b__1()
   bei Microsoft.Mashup.Common.SafeExceptions.IgnoreSafeExceptions(IEngineHost host, IHostTrace trace, Action action)
   bei Microsoft.Mashup.Evaluator.EvaluationHost.TryReportException(IHostTrace trace, IEngineHost engineHost, IMessageChannel channel, Exception exception)
   bei Microsoft.Mashup.Evaluator.EvaluationHost.TryHandleException(Exception exception)
   bei Microsoft.Mashup.Evaluator.SafeThread2.HandleException(Exception e)
   bei Microsoft.Mashup.Evaluator.SafeThread2.<>c__DisplayClass9_0.<CreateAction>b__0(Object o)
   bei Microsoft.Mashup.Container.EvaluationContainerMain.SafeRun(String[] args)
   bei Microsoft.Mashup.Container.BootstrapAppDomainManager.Execute(String[] argv)

InnerException1.Invocation Stack Trace:
   bei Microsoft.Mashup.Host.Document.ExceptionExtensions.GetCurrentInvocationStackTrace()
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromException(Exception e, String prefix)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.CreateAdditionalErrorInfo(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo..ctor(String message, Exception exception, Nullable`1 stackTraceInfo, String messageDetail)
   bei Microsoft.Mashup.Client.UI.Shared.IUIHostExtensions.RaiseErrorDialog(IUIHost uiHost, IWindowHandle activeWindow, FeedbackPackageInfo feedbackPackageInfo, Exception e, LocalizedString dialogTitle, LocalizedString dialogMessage, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.Excel.Native.NativeUserFeedbackServices.ReportException(IWindowHandle activeWindow, IUIHost uiHost, FeedbackPackageInfo feedbackPackageInfo, Exception e, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.<>c__DisplayClass14_0.<HandleException>b__0()
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.HandleException(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.HandleImportEvaluationException(ExceptionResult exceptionView, Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.OnGetPreviewResult(PreviewResult preview, Query query, String sourceID, String formulaTitle, Nullable`1 explicitImportDestination, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass65_0.<GetPreviewResult>b__1()
   bei Microsoft.Mashup.Client.Excel.Shim.NativeWorkbookStorageServices.Microsoft.Mashup.Client.Excel.Shim.IDeferredStorageInvoker.InvokeDeferredStorageAction(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.InvokeCoauthAction(IWorkbook workbook, UndoableActionType actionType, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.NotifyGetDataPresence(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass59_0.<InvokeOnWorkbook>b__0(IWorkbook workbook, IWindowContext windowContext)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.InvokeOnWorkbook[T](Func`3 action, T defaultValue)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.<>c__DisplayClass90_0.<OnQuerySettingsResolved>b__0()
   bei Microsoft.Mashup.Host.Document.ExceptionHandlerExtensions.HandleExceptions(IExceptionHandler exceptionHandler, Action action)
   bei System.RuntimeMethodHandle.InvokeMethod(Object target, Object[] arguments, Signature sig, Boolean constructor)
   bei System.Reflection.RuntimeMethodInfo.UnsafeInvokeInternal(Object obj, Object[] parameters, Object[] arguments)
   bei System.Delegate.DynamicInvokeImpl(Object[] args)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackDo(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackHelper(Object obj)
   bei System.Threading.ExecutionContext.RunInternal(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state)
   bei System.Windows.Forms.Control.InvokeMarshaledCallback(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbacks()
   bei System.Windows.Forms.Control.WndProc(Message& m)
   bei System.Windows.Forms.NativeWindow.Callback(IntPtr hWnd, Int32 msg, IntPtr wparam, IntPtr lparam)


InnerException2.Stack Trace Message:
Das SafeHandle wurde geschlossen.

InnerException2.Stack Trace:
   bei System.Runtime.InteropServices.SafeHandle.DangerousAddRef(Boolean& success)
   bei System.StubHelpers.StubHelpers.SafeHandleAddRef(SafeHandle pHandle, Boolean& success)
   bei Microsoft.Win32.Win32Native.SetEvent(SafeWaitHandle handle)
   bei System.Threading.EventWaitHandle.Set()
   bei Microsoft.Mashup.Evaluator.FirewallDocumentEvaluator.<>c__DisplayClass5_0.<BeginGetResult>b__0(EvaluationResult2`1 result)
   bei Microsoft.Mashup.Evaluator.FirewallDocumentEvaluator.BeginGetResultInternal[T](DocumentEvaluationParameters parameters, Action`1 callback)
   bei Microsoft.Mashup.Evaluator.Interface.IDocumentEvaluatorExtensions.GetResult[T](IDocumentEvaluator`1 evaluator, DocumentEvaluationParameters parameters)
   bei Microsoft.Mashup.Evaluator.RemoteDocumentEvaluator.Service.OnBeginGetResult[T](IMessageChannel channel, BeginGetResultMessage message, Action`1 action)
   bei Microsoft.Mashup.Evaluator.RemoteDocumentEvaluator.Service.OnBeginGetPreviewValueSource(IMessageChannel channel, BeginGetPreviewValueSourceMessage message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.ChannelMessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.Dispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.OnMessageWithUnknownChannel(IMessageChannel baseChannel, MessageWithUnknownChannel messageWithUnknownChannel)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.ChannelMessenger.ChannelMessageHandlers.TryDispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.MessageHandlers.Dispatch(IMessageChannel channel, Message message)
   bei Microsoft.Mashup.Evaluator.EvaluationHost.Run()
   bei Microsoft.Mashup.Container.EvaluationContainerMain.Run(Object args)
   bei Microsoft.Mashup.Evaluator.SafeThread2.<>c__DisplayClass9_0.<CreateAction>b__0(Object o)

InnerException2.Invocation Stack Trace:
   bei Microsoft.Mashup.Host.Document.ExceptionExtensions.GetCurrentInvocationStackTrace()
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromException(Exception e, String prefix)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.GetFeedbackValuesFromInnerExceptions(Exception e, Int32 depth)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo.CreateAdditionalErrorInfo(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.FeedbackErrorInfo..ctor(String message, Exception exception, Nullable`1 stackTraceInfo, String messageDetail)
   bei Microsoft.Mashup.Client.UI.Shared.IUIHostExtensions.RaiseErrorDialog(IUIHost uiHost, IWindowHandle activeWindow, FeedbackPackageInfo feedbackPackageInfo, Exception e, LocalizedString dialogTitle, LocalizedString dialogMessage, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.Excel.Native.NativeUserFeedbackServices.ReportException(IWindowHandle activeWindow, IUIHost uiHost, FeedbackPackageInfo feedbackPackageInfo, Exception e, Boolean useGDICapture)
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.<>c__DisplayClass14_0.<HandleException>b__0()
   bei Microsoft.Mashup.Client.UI.Shared.UnexpectedExceptionHandler.HandleException(Exception e)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.HandleImportEvaluationException(ExceptionResult exceptionView, Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.OnGetPreviewResult(PreviewResult preview, Query query, String sourceID, String formulaTitle, Nullable`1 explicitImportDestination, Boolean isNewQuery, Boolean isFromEditor)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass65_0.<GetPreviewResult>b__1()
   bei Microsoft.Mashup.Client.Excel.Shim.NativeWorkbookStorageServices.Microsoft.Mashup.Client.Excel.Shim.IDeferredStorageInvoker.InvokeDeferredStorageAction(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.InvokeCoauthAction(IWorkbook workbook, UndoableActionType actionType, Action action)
   bei Microsoft.Mashup.Client.Excel.Shim.NativeCoAuthServices.NotifyGetDataPresence(IWorkbook workbook, Action action)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.<>c__DisplayClass59_0.<InvokeOnWorkbook>b__0(IWorkbook workbook, IWindowContext windowContext)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.InvokeOnWorkbook[T](Func`3 action, T defaultValue)
   bei Microsoft.Mashup.Client.Excel.ExcelDataImporter.GetPreviewResult(Query query, String sourceID, String formulaTitle, Boolean isNewQuery, Boolean isFromEditor, Nullable`1 explicitImportDestination)
   bei Microsoft.Mashup.Client.UI.Shared.DataImporter.<>c__DisplayClass90_0.<OnQuerySettingsResolved>b__0()
   bei Microsoft.Mashup.Host.Document.ExceptionHandlerExtensions.HandleExceptions(IExceptionHandler exceptionHandler, Action action)
   bei System.RuntimeMethodHandle.InvokeMethod(Object target, Object[] arguments, Signature sig, Boolean constructor)
   bei System.Reflection.RuntimeMethodInfo.UnsafeInvokeInternal(Object obj, Object[] parameters, Object[] arguments)
   bei System.Delegate.DynamicInvokeImpl(Object[] args)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackDo(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbackHelper(Object obj)
   bei System.Threading.ExecutionContext.RunInternal(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state, Boolean preserveSyncCtx)
   bei System.Threading.ExecutionContext.Run(ExecutionContext executionContext, ContextCallback callback, Object state)
   bei System.Windows.Forms.Control.InvokeMarshaledCallback(ThreadMethodEntry tme)
   bei System.Windows.Forms.Control.InvokeMarshaledCallbacks()
   bei System.Windows.Forms.Control.WndProc(Message& m)
   bei System.Windows.Forms.NativeWindow.Callback(IntPtr hWnd, Int32 msg, IntPtr wparam, IntPtr lparam)


Supports Premium Content:
True

Formulas:


section Section1;

shared Abfrage1 = let
    Quelle = Odbc.Query("dsn=SAP_DAA", "SELECT * FROM MARA")
in
    Quelle;
