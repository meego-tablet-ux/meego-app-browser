//---------------------------------------------------------------------------------------
//  $Id: OCMockRecorder.m 50 2009-07-16 06:48:19Z erik $
//  Copyright (c) 2004-2009 by Mulle Kybernetik. See License file for details.
//---------------------------------------------------------------------------------------

#import <objc/runtime.h>
#import <OCMock/OCMockRecorder.h>
#import <OCMock/OCMArg.h>
#import <OCMock/OCMConstraint.h>
#import "OCMPassByRefSetter.h"
#import "OCMReturnValueProvider.h"
#import "OCMBoxedReturnValueProvider.h"
#import "OCMExceptionReturnValueProvider.h"
#import "OCMIndirectReturnValueProvider.h"
#import "OCMNotificationPoster.h"
#import "NSInvocation+OCMAdditions.h"

@interface NSObject(HCMatcherDummy)
- (BOOL)matches:(id)item;
@end

#pragma mark  -


@implementation OCMockRecorder

#pragma mark  Initialisers, description, accessors, etc.

- (id)initWithSignatureResolver:(id)anObject
{
	signatureResolver = anObject;
	invocationHandlers = [[NSMutableArray alloc] init];
	return self;
}

- (void)dealloc
{
	[recordedInvocation release];
	[invocationHandlers release];
	[super dealloc];
}

- (NSString *)description
{
	return [recordedInvocation invocationDescription];
}

- (void)releaseInvocation
{
	[recordedInvocation release];
	recordedInvocation = nil;
}


#pragma mark  Recording invocation handlers

- (id)andReturn:(id)anObject
{
	[invocationHandlers addObject:[[[OCMReturnValueProvider alloc] initWithValue:anObject] autorelease]];
	return self;
}

- (id)andReturnValue:(NSValue *)aValue
{
	[invocationHandlers addObject:[[[OCMBoxedReturnValueProvider alloc] initWithValue:aValue] autorelease]];
	return self;
}

- (id)andThrow:(NSException *)anException
{
	[invocationHandlers addObject:[[[OCMExceptionReturnValueProvider alloc] initWithValue:anException] autorelease]];
	return self;
}

- (id)andPost:(NSNotification *)aNotification
{
	[invocationHandlers addObject:[[[OCMNotificationPoster alloc] initWithNotification:aNotification] autorelease]];
	return self;
}

- (id)andCall:(SEL)selector onObject:(id)anObject
{
	[invocationHandlers addObject:[[[OCMIndirectReturnValueProvider alloc] initWithProvider:anObject andSelector:selector] autorelease]];
	return self;
}

- (NSArray *)invocationHandlers
{
	return invocationHandlers;
}


#pragma mark  Recording the actual invocation

- (NSMethodSignature *)methodSignatureForSelector:(SEL)aSelector
{
	return [signatureResolver methodSignatureForSelector:aSelector];
}

- (void)forwardInvocation:(NSInvocation *)anInvocation
{
	if(recordedInvocation != nil)
		[NSException raise:NSInternalInconsistencyException format:@"Recorder received two methods to record."];
	[anInvocation setTarget:nil];
	[anInvocation retainArguments];
	recordedInvocation = [anInvocation retain];
}



#pragma mark  Checking the invocation

- (BOOL)matchesInvocation:(NSInvocation *)anInvocation
{
	id  recordedArg, passedArg;
	int i, n;
	
	if([anInvocation selector] != [recordedInvocation selector])
		return NO;
	
	n = [[recordedInvocation methodSignature] numberOfArguments];
	for(i = 2; i < n; i++)
	{
		recordedArg = [recordedInvocation getArgumentAtIndexAsObject:i];
		if([recordedArg isKindOfClass:[NSValue class]])
			recordedArg = [OCMArg resolveSpecialValues:recordedArg];
		passedArg = [anInvocation getArgumentAtIndexAsObject:i];
		
		if([recordedArg isKindOfClass:[OCMConstraint class]])
		{	
			if([recordedArg evaluate:passedArg] == NO)
				return NO;
		}
		else if([recordedArg isKindOfClass:[OCMPassByRefSetter class]])
		{
			// side effect but easier to do here than in handleInvocation
			*(id *)[passedArg pointerValue] = [(OCMPassByRefSetter *)recordedArg value];
		}
		else if([recordedArg conformsToProtocol:objc_getProtocol("HCMatcher")])
		{
			if([recordedArg matches:passedArg] == NO)
				return NO;
		}
		else
		{
			if([recordedArg class] != [passedArg class])
				return NO;
			if(([recordedArg class] == [NSNumber class]) && 
				([(NSNumber*)recordedArg compare:(NSNumber*)passedArg] != NSOrderedSame))
				return NO;
			if(([recordedArg isEqual:passedArg] == NO) &&
				!((recordedArg == nil) && (passedArg == nil)))
				return NO;
		}
	}
	return YES;
}




@end
